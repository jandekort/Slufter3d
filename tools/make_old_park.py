#!/usr/bin/env python3
# Slufter3D - reads the buildings of the old park (1957 to 1996) off the maps of earlier years
# Author: Jan de Kort
# Created: 2026-11-21
#
# Usage: python3 tools/make_old_park.py
#
# raw/topotijdreis holds screenshots of the topographic map 1:25 000 of 1965, 1983 and 1994, each beside the
# map of 1998 at the same scale. On those maps every bungalow of the old park is a dot. This tool
#   1. cuts each screenshot into its two panels and finds the grid lines that cross in them: RD x = 119000 and
#      y = 575000;
#   2. takes the scale from the panel of 1998: its buildings are fitted onto the houses of data/houses.json;
#   3. finds the dots of each old map and gives them RD coordinates: the scale of the 1998 panel, anchored on
#      the old map's own grid crossing;
#   4. takes the dots of 1983, with those that data/old_park_corrections.json adds, as the buildings the
#      topographic maps know;
#   5. fits the two maps of the park's own brochures (raw/park_brochures, read by hand into
#      data/old_park_brochures.json) onto them: the earlier one by a linear fit that pairs each numbered bungalow
#      with a dot, the later one, bent where the brochure folds, by a quadratic fit through the bungalows of the
#      same number;
#   6. decides. The earlier park map (the later one already shows bungalows that came after the old park, and
#      only serves to compare by) says which bungalows there were, their number and their type; the topographic maps
#      say where: a bungalow stands on the dot it is paired with. One without a dot of 1983 takes a dot of 1994
#      or 1965 near it, and failing that its place on the park map. Dots that no bungalow takes are not
#      bungalows: the old reception, Krimweg 73, buildings of the camping.
#   7. adds what data/old_park_corrections.json knows besides: names, fronts, what kind each type is.
# Writes data/old_park.json and, to check it by, review/old_park/: the dots found on each map, everything on the
# relief of today (every map's idea of every bungalow, and the place decided on), and a table.
#
# How good: the two houses that still stand lie 2 m and 4 m from their dot of 1983. The dots of 1994 lie 4 m
# (median) from those of 1983; those of 1965 about 11 m, after a shift of 7 m is taken out, and are hard to tell
# from the red shading of the dunes. The park maps, though drawn by hand, lie 4 m (median) from the dots after
# their fit; 77 of the 81 bungalows of the earlier one have a dot of 1983 within 15 m.
#
# Needs: numpy, scipy, pillow, rasterio (for the relief of the review map)
import json
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage
from scipy.optimize import linear_sum_assignment

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
MAPS = os.path.join(ROOT, "raw", "topotijdreis")
HOUSES = os.path.join(ROOT, "data", "houses.json")
LANDMARKS = os.path.join(ROOT, "data", "landmarks.json")
CORRECTIONS = os.path.join(ROOT, "data", "old_park_corrections.json")
BROCHURES = os.path.join(ROOT, "data", "old_park_brochures.json")
TYPE_MODELS = os.path.join(ROOT, "data", "type_models.json")
GROUND = os.path.join(ROOT, "raw", "ahn", "park_dtm_05m.tif")
OUT = os.path.join(ROOT, "data", "old_park.json")
REVIEW = os.path.join(ROOT, "review", "old_park")

GRID = (119000.0, 575000.0)     # the grid lines that cross in every panel [m RD]
MASTER = 1983                   # the map whose dots are the list of buildings
# The panels of each screenshot: old map, map of 1998, as (left, right) columns; rows from top to bottom
PANELS = {
    1965: {"file": "Map_1965.png", "old": (131, 1430), "new": (1437, 2735), "rows": (0, -25)},
    1983: {"file": "Map_1983.png", "old": (131, 1430), "new": (1437, 2735), "rows": (0, -25)},
    1994: {"file": "Map_1994.png", "old": (130, 1458), "new": (1466, 2794), "rows": (122, 1350)}}
UNKNOWN = "Unknown"    # what is not known of a building
PAIRED = 15.0          # a bungalow of the park map and a dot of 1983 this near are one building [m]
OTHER_MAP = 15.0       # a bungalow without one takes a dot of 1994, or else of 1965, up to this far [m]
SAME_DOT = 8.0         # a dot of 1994 this near a bungalow that has its place is that bungalow [m]

def panels(year):
    """The old map and the map of 1998 of a screenshot, as arrays of red, green, blue"""
    p = PANELS[year]
    a = np.asarray(Image.open(os.path.join(MAPS, p["file"])).convert("RGB")).astype(float)
    rows = slice(p["rows"][0], p["rows"][1] if p["rows"][1] > 0 else a.shape[0] + p["rows"][1])
    return a[rows, p["old"][0]:p["old"][1]], a[rows, p["new"][0]:p["new"][1]]


def gridCrossing(panel):
    """Where the two grid lines of a panel cross [px]: the column and the row with the most dark pixels"""
    dark = panel.max(-1) < 150.0
    column = ndimage.uniform_filter1d(dark.sum(0).astype(float), 5)
    row = ndimage.uniform_filter1d(dark.sum(1).astype(float), 5)
    return float(np.argmax(column)), float(np.argmax(row))


def blobs(mask, smallest, largest, widest):
    """Centres of the connected patches of a mask of between smallest and largest pixels [px]"""
    labels, count = ndimage.label(mask)
    found = []
    for i, box in enumerate(ndimage.find_objects(labels)):
        patch = labels[box] == i + 1
        if smallest <= patch.sum() <= largest and max(patch.shape) < widest:
            cy, cx = ndimage.center_of_mass(patch)
            found.append((box[1].start + cx, box[0].start + cy))
    return np.array(found)


def scaleOf(panel, houses):
    """Pixels per metre of a panel of 1998, by fitting its black buildings onto the houses of today"""
    black = (panel.sum(-1) < 200.0) & (panel.max(-1) - panel.min(-1) < 60.0)
    marks = blobs(black, 25, 900, 45)
    gx, gy = gridCrossing(panel)
    best = None
    for start in np.arange(1.80, 2.16, 0.05):       # the fit keeps the pairs it starts with: try several scales
        A = np.array([[1.0 / start, 0.0], [0.0, -1.0 / start]])
        t = np.array([GRID[0] - gx / start, GRID[1] + gy / start])
        for step in range(6):
            world = marks @ A.T + t
            distance = np.linalg.norm(world[:, None] - houses[None], axis=2)
            nearest = distance.argmin(1)
            near = distance.min(1) < (25.0 if step < 2 else 12.0)
            solution = np.linalg.lstsq(np.c_[marks[near], np.ones(near.sum())], houses[nearest[near]], rcond=None)[0]
            A, t = solution[:2].T, solution[2]
        # Judged by how many houses it fits and how well
        fitted = int(near.sum())
        residual = float(np.median(distance.min(1)[near]))
        scale = 0.5 * (1.0 / np.linalg.norm(A[:, 0]) + 1.0 / np.linalg.norm(A[:, 1]))
        if best is None or fitted / (1.0 + residual) > best[0]:
            best = (fitted / (1.0 + residual), scale, fitted, residual)
    return best[1], best[2], best[3]


def dotsOf(year, panel):
    """The dots of buildings on an old map [px]: red on 1965 and 1983, black on 1994"""
    r, g, b = panel[..., 0], panel[..., 1], panel[..., 2]
    if year == 1983:
        mask = ndimage.binary_opening((r > 170) & (g < 110) & (b < 110))
        return blobs(mask, 40, 900, 60)
    if year == 1965:
        # The dunes are shaded in the same red: only what survives a wide opening is a building
        mask = ndimage.binary_opening((r > 165) & (g < 125) & (b < 115) & (r - g > 75), structure=np.ones((7, 7)))
        return blobs(mask, 45, 900, 50)
    # 1994: black dots, blurred; the letters of the labels come out as dots too, which is why 1994 only confirms
    soft = ndimage.gaussian_filter(panel.sum(-1), 1.2)
    mask = (soft < 300.0) & (panel.max(-1) - panel.min(-1) < 70.0)
    return blobs(mask, 12, 260, 25)


def review(name, panel, marks):
    image = Image.fromarray(panel.astype(np.uint8))
    draw = ImageDraw.Draw(image)
    for x, y in marks:
        draw.ellipse([x - 13, y - 13, x + 13, y + 13], outline=(0, 0, 255), width=2)
    image.save(os.path.join(REVIEW, name))


def linearFit(pixels, world):
    """The linear map from pixels to RD that fits pairs of points best"""
    solution = np.linalg.lstsq(np.c_[pixels, np.ones(len(pixels))], world, rcond=None)[0]
    return lambda points: np.c_[np.asarray(points, float), np.ones(len(points))] @ solution


def quadraticFit(pixels, world):
    """As linearFit, with the squares: for a page that is bent"""
    def terms(points):
        points = np.asarray(points, float)
        x, y = (points[:, 0] - 800.0) / 400.0, (points[:, 1] - 1300.0) / 400.0
        return np.c_[np.ones(len(points)), x, y, x * x, x * y, y * y]
    solution = np.linalg.lstsq(terms(pixels), world, rcond=None)[0]
    return lambda points: terms(points) @ solution


def pairWithDots(parkMap, start, dots, tower):
    """Fits the earlier park map onto the dots; returns the fit and, per number, the index of its dot"""
    bungalows = parkMap["bungalows"]
    pixels = np.array([[b["x_px"], b["y_px"]] for b in bungalows], float)
    numbers = [b["number"] for b in bungalows]
    anchor = np.array([parkMap["tower_px"]] * 3, float)        # the tower counts for three
    fit = linearFit(np.r_[[pixels[numbers.index(s["number"])] for s in start], anchor[:1]],
                    np.r_[[[s["x"], s["y"]] for s in start], [tower]])
    for gate in (60.0, 45.0, 35.0, 28.0, 22.0, 18.0, PAIRED, PAIRED):
        distance = np.linalg.norm(fit(pixels)[:, None] - dots[None], axis=2)
        rows, columns = linear_sum_assignment(np.minimum(distance, gate))
        near = distance[rows, columns] < gate
        fit = linearFit(np.r_[pixels[rows[near]], anchor], np.r_[dots[columns[near]], [tower] * 3])
    return fit, {numbers[r]: int(c) for r, c in zip(rows[near], columns[near])}


def reliefMap(buildings, found, parkPlaces, laterOnly, unused, houses, tower):
    """Everything on the relief of today: every map's idea of every bungalow, and the place decided on"""
    import rasterio
    from rasterio.windows import from_bounds
    x0, x1, y0, y1 = 118540.0, 119180.0, 574620.0, 575320.0
    with rasterio.open(GROUND) as source:
        z = source.read(1, window=from_bounds(x0, y0, x1, y1, source.transform)).astype(float)
    missing = z > 1.0e6
    z[missing] = np.nan
    filled = np.where(np.isnan(z), np.nanmedian(z), z)
    north, east = np.gradient(filled, 0.5)
    shade = np.clip(0.55 - 0.9 * east + 0.9 * north, 0.0, 1.3) / 1.3
    high = np.clip((filled - 2.0) / 12.0, 0.0, 1.0)
    colour = np.stack([0.60 + 0.35 * high, 0.80 - 0.15 * high, 0.56 - 0.10 * high], -1) * (0.5 + 0.5 * shade[..., None])
    colour = 0.45 + 0.55 * colour      # pale, so that the marks stand out
    colour[missing] = (0.86, 0.86, 0.86)
    zoom = 3
    image = Image.fromarray((np.clip(colour, 0.0, 1.0) * 255).astype(np.uint8)).resize((z.shape[1] * zoom, z.shape[0] * zoom))
    draw = ImageDraw.Draw(image)
    try:
        bold = ImageFont.truetype("DejaVuSans-Bold.ttf", 17)
        plain = ImageFont.truetype("DejaVuSans.ttf", 14)
    except OSError:
        bold = plain = ImageFont.load_default()
    at = lambda x, y: ((x - x0) * 2.0 * zoom, (y1 - y) * 2.0 * zoom)
    inside = lambda x, y: x0 < x < x1 and y0 < y < y1
    for number, x, y in houses:
        if inside(x, y):
            px, py = at(x, y)
            draw.text((px - 8, py + 14), str(number), fill=(150, 150, 150), font=plain)
    for x, y in found[1965]:
        if inside(x, y):
            px, py = at(x, y)
            draw.line([px - 7, py - 7, px + 7, py + 7], fill=(230, 130, 0), width=2)
            draw.line([px - 7, py + 7, px + 7, py - 7], fill=(230, 130, 0), width=2)
    for x, y in found[1994]:
        if inside(x, y):
            px, py = at(x, y)
            draw.ellipse([px - 15, py - 15, px + 15, py + 15], outline=(0, 0, 0), width=2)
    for x, y in found[MASTER]:
        if inside(x, y):
            px, py = at(x, y)
            draw.ellipse([px - 6, py - 6, px + 6, py + 6], fill=(215, 30, 30))
    for x, y in unused:
        px, py = at(x, y)
        draw.rectangle([px - 12, py - 12, px + 12, py + 12], outline=(215, 30, 30), width=2)
    fills = {"A4": (205, 60, 40), "A6": (110, 140, 40), "B4": (235, 200, 60), "B6": (60, 110, 200)}
    for b in laterOnly:
        px, py = at(b["x"], b["y"])
        draw.rectangle([px - 7, py - 7, px + 7, py + 7], outline=(160, 0, 200), width=2)
        draw.text((px + 10, py - 8), "%s %s" % (b["number"] if b["number"] != UNKNOWN else "?", b["type"]), fill=(120, 0, 160), font=plain)
    for b in buildings:
        px, py = at(b["x"], b["y"])
        for which, colour in ((0, (0, 110, 255)), (1, (160, 0, 200))):
            place = parkPlaces[which].get(b["id"])
            if place is not None and inside(*place):
                qx, qy = at(*place)
                draw.line([px, py, qx, qy], fill=colour, width=2)
                if which == 0:
                    draw.polygon([qx, qy - 7, qx - 6, qy + 5, qx + 6, qy + 5], fill=colour)
                else:
                    draw.rectangle([qx - 5, qy - 5, qx + 5, qy + 5], fill=colour)
    for b in buildings:
        px, py = at(b["x"], b["y"])
        draw.ellipse([px - 11, py - 11, px + 11, py + 11], outline=fills[b["type"]], width=5)
        label = str(b["number"]) if b["number"] != UNKNOWN else "?"
        draw.text((px + 13, py - 22), label if b["name"] == UNKNOWN else "%s %s" % (label, b["name"]), fill=(0, 0, 0), font=bold)
    px, py = at(*tower)
    draw.rectangle([px - 9, py - 9, px + 9, py + 9], outline=(0, 0, 0), width=3)
    draw.text((px + 12, py + 6), "tower", fill=(0, 0, 0), font=bold)
    legend = [
        "TOPOGRAPHIC MAPS   red dot: 1983    black ring: 1994    orange cross: 1965    red square: a dot no bungalow takes",
        "PARK MAPS, fitted   blue triangle: the earlier one    purple square: the later one    each tied to the place decided on",
        "DECIDED   thick ring with the number: red A4, green A6, yellow B4, blue B6",
        "hollow purple square with number and type: on the later park map only, so not of the old park",
        "grey numbers: the houses of today, on the relief of today (AHN)"]
    draw.rectangle([8, 8, 1010, 20 + 24 * len(legend)], fill=(255, 255, 255), outline=(0, 0, 0))
    for k, line in enumerate(legend):
        draw.text((16, 14 + 24 * k), line, fill=(0, 0, 0), font=plain)
    image.save(os.path.join(REVIEW, "old_park_all_maps.png"))


def main():
    os.makedirs(REVIEW, exist_ok=True)
    today = [h for h in json.load(open(HOUSES))["houses"] if h["kind"] == "building"]
    houses = np.array([[h["x"], h["y"]] for h in today])
    numbered = [(h["nr"], h["x"], h["y"]) for h in today if h["nr"]]
    corrections = json.load(open(CORRECTIONS))
    brochures = json.load(open(BROCHURES))
    doors = {name: entry.get("door") for name, entry in json.load(open(TYPE_MODELS))["types"].items()}
    tower = np.array(json.load(open(LANDMARKS))["lookout_tower"]["rd"], float)

    found = {}
    report = []
    for year in sorted(PANELS):
        old, new = panels(year)
        scale, matched, residual = scaleOf(new, houses)
        gx, gy = gridCrossing(old)
        marks = dotsOf(year, old)
        found[year] = np.c_[GRID[0] + (marks[:, 0] - gx) / scale, GRID[1] - (marks[:, 1] - gy) / scale]
        review("dots_%d.png" % year, old, marks)
        report.append("%d: %.3f px/m (%d houses of 1998 fitted, median %.1f m), grid crossing at (%.0f, %.0f) px, %d dots"
                      % (year, scale, matched, residual, gx, gy, len(marks)))
    # The map of 1965 lies a little off: take out its median shift against the master
    distance = np.linalg.norm(found[1965][:, None] - found[MASTER][None], axis=2)
    pairs = distance.min(1) < 20.0
    shift = np.median(found[MASTER][distance.argmin(1)[pairs]] - found[1965][pairs], axis=0)
    found[1965] = found[1965] + shift
    report.append("1965 shifted by (%.1f, %.1f) m onto %d" % (shift[0], shift[1], MASTER))

    hidden = corrections["hidden"]
    dots = np.r_[found[MASTER], [[h["x"], h["y"]] for h in hidden]]
    dotSource = ["map of %d" % MASTER] * len(found[MASTER]) + ["map of %d, read by eye" % h["map"] for h in hidden]

    # The earlier park map: every bungalow with its dot
    first, second = brochures["maps"]
    fitFirst, paired = pairWithDots(first, brochures["start"], dots, tower)
    byNumber = {}       # number -> what the park maps say of it
    for which, parkMap in enumerate((first, second)):
        for k, b in enumerate(parkMap["bungalows"]):
            key = b["number"] if b["number"] != UNKNOWN else "x%d" % k
            entry = byNumber.setdefault(key, {"number": b["number"], "types": [None, None], "px": [None, None]})
            entry["types"][which] = b["type"]
            entry["px"][which] = (b["x_px"], b["y_px"])
    firstPlace = {key: fitFirst([e["px"][0]])[0] for key, e in byNumber.items() if e["px"][0]}
    place, source = {}, {}
    for key in firstPlace:
        if key in paired:
            place[key], source[key] = dots[paired[key]], dotSource[paired[key]]
    taken = set(paired.values())
    for key in sorted(k for k in firstPlace if k not in place):
        spot = firstPlace[key]
        for year in (1994, 1965):
            away = np.linalg.norm(found[year] - spot, axis=1)
            if away.min() < OTHER_MAP:
                place[key], source[key] = found[year][int(away.argmin())], "map of %d" % year
                break
        else:
            place[key], source[key] = spot, "park map"
    # The later park map, through the bungalows of the same number
    common = [key for key, e in byNumber.items() if e["px"][1] and key in paired]
    fitSecond = quadraticFit([byNumber[key]["px"][1] for key in common], [dots[paired[key]] for key in common])
    secondPlace = {key: fitSecond([e["px"][1]])[0] for key, e in byNumber.items() if e["px"][1]}
    # What only the later map has came after the old park (the Comfort bungalows): kept apart, to compare by
    laterOnly = [{"number": byNumber[key]["number"], "type": byNumber[key]["types"][1],
                  "x": round(float(secondPlace[key][0]), 1), "y": round(float(secondPlace[key][1]), 1)}
                 for key in sorted(secondPlace, key=lambda key: (isinstance(key, str), key)) if key not in place]
    for name, fit, where in (("earlier", fitFirst, firstPlace), ("later", fitSecond, secondPlace)):
        off = [np.linalg.norm(where[key] - dots[paired[key]]) for key in where if key in paired]
        report.append("the %s park map: %d bungalows with a dot of %d, median %.1f m from it, at most %.1f m"
                      % (name, len(off), MASTER, np.median(off), max(off)))
    unused = [dots[j] for j in range(len(dots)) if j not in taken]

    order = sorted(place)
    buildings = []
    notes = []
    for key in order:
        e = byNumber[key]
        kind = e["types"][0]        # the earlier map is the one of the old park
        if e["types"][0] and e["types"][1] and e["types"][0] != e["types"][1]:
            notes.append("%s is %s on the earlier park map, which is followed, and %s on the later one" % (key, e["types"][0], e["types"][1]))
        x, y = place[key]
        apart = np.hypot(houses[:, 0] - x, houses[:, 1] - y)
        nearest = today[int(apart.argmin())]
        what = corrections["types"][kind]
        b = {"id": "OP%s" % key, "number": e["number"], "name": UNKNOWN, "kind": what["kind"], "type": kind,
             "front_azimuth": UNKNOWN, "x": round(float(x), 1), "y": round(float(y), 1)}
        if "since" in what:
            b["since"] = what["since"]
        b["source"] = {"position": source[key], "on_later_park_map": e["px"][1] is not None,
                       "park_maps_off": [None if where.get(key) is None else round(float(np.linalg.norm(where[key] - place[key])), 1)
                                         for where in (firstPlace, secondPlace)]}
        b["today"] = {"house": nearest["nr"] or nearest["type"], "distance": round(float(apart.min()), 1)}
        buildings.append(b)
    known = {b["number"]: b for b in buildings}
    for entry in corrections["named"]:
        b = known[entry["number"]]
        for field in ("name", "front_azimuth"):
            if field in entry:
                b[field] = entry[field]
        if entry.get("front_from_today"):
            standing = today[int(np.argmin(np.hypot(houses[:, 0] - b["x"], houses[:, 1] - b["y"])))]
            b["front_azimuth"] = standing["front_azimuth"]
        if "door_seen" in entry and entry["door_seen"] != doors[b["type"]]:
            notes.append("%s: its door is %s in a photograph, but type %s has a %s door"
                         % (entry["number"], entry["door_seen"], b["type"], doors[b["type"]]))
        b["evidence"] = entry["why"]

    out = {
        "note": "The buildings of the park as it was from 1957 to 1996. Which there were, their number and their type "
                "(A4, A6, B4, B6) are those of the earlier of the park's own two maps (raw/park_brochures); their positions those "
                "of the topographic maps (raw/topotijdreis) where these have them, RD New (EPSG:28992), metres, good to "
                "about 5 m. Made by tools/make_old_park.py. The door, and so the model, follows from the type in "
                "data/type_models.json. kind: Bungalow or Chalet. front_azimuth: the way the front door faces, degrees "
                "clockwise from north. What is not known is 'Unknown'. source.position: where the position is from; "
                "park_maps_off: how far the earlier and the later park map, fitted, have it from there [m]. today: the "
                "nearest house of today. later_map_only: what only the later park map shows, the bungalows that came after "
                "the old park (type E): not part of the old park, their fitted positions are rough.",
        "source": "Maps of the park's brochures; Topotijdreis, Kadaster: TOP25 of %s" % ", ".join(str(y) for y in sorted(PANELS)),
        "report": report, "notes": notes,
        "later_map_only": laterOnly,
        "not_bungalows": [{"x": round(float(x), 1), "y": round(float(y), 1)} for x, y in unused],
        "buildings": buildings}
    with open(OUT, "w") as f:
        json.dump(out, f, indent=1, ensure_ascii=False)
        f.write("\n")

    counts = {}
    for b in buildings:
        counts[b["source"]["position"].split(",")[0]] = counts.get(b["source"]["position"].split(",")[0], 0) + 1
    lines = ["# The old park, 1957 to 1996", "", "Made by tools/make_old_park.py from raw/topotijdreis and raw/park_brochures.", ""]
    lines += ["- " + r for r in report] + [""] + ["- " + n for n in notes] + [
        "", "%d buildings. Their positions: %s." % (len(buildings), ", ".join("%d from the %s" % (n, s) for s, n in sorted(counts.items()))),
        "", "Dots of the topographic maps that no bungalow takes: %s." % ", ".join("(%.0f, %.0f)" % (x, y) for x, y in unused),
        "", "| number | name | type | kind | front | x | y | position from | earlier park map off | later off | house today | m |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for b in buildings:
        off = ["" if v is None else "%.0f" % v for v in b["source"]["park_maps_off"]]
        lines.append("| %s | %s | %s | %s | %s | %.0f | %.0f | %s | %s | %s | %s | %.0f |" % (
            b["number"], "" if b["name"] == UNKNOWN else b["name"], b["type"], b["kind"], b["front_azimuth"], b["x"], b["y"],
            b["source"]["position"], off[0], off[1], b["today"]["house"], b["today"]["distance"]))
    with open(os.path.join(REVIEW, "old_park.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    ids = lambda where: {"OP%s" % key: value for key, value in where.items()}
    reliefMap(buildings, found, (ids(firstPlace), ids(secondPlace)), laterOnly, unused, numbered, tower)
    for line in report + notes:
        print(line)
    print("%d buildings -> %s; review in %s" % (len(buildings), os.path.relpath(OUT, ROOT), os.path.relpath(REVIEW, ROOT)))


if __name__ == "__main__":
    main()
