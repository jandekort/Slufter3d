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
#   4. takes the map of 1983 as the list of buildings, since nearly nothing hides its dots, and notes for each
#      whether 1994 and 1965 have it too;
#   5. adds and names what data/old_park_corrections.json says: dots under a label, and what the photographs
#      of raw/park_1957_1996 tell (see tools/check_old_park_photos.py).
# Writes data/old_park.json and, to check it by, review/old_park/: the dots found on each map, the buildings on
# the relief of today with the houses of today, and a table.
#
# How good: the two houses that still stand lie 2 m and 4 m from their dot of 1983. The dots of 1994 lie 4 m
# (median) from those of 1983; those of 1965 about 11 m, after a shift of 7 m is taken out, and are hard to tell
# from the red shading of the dunes, so 1965 only confirms. A dot is a symbol of about 8 m, drawn by hand.
#
# Needs: numpy, scipy, pillow, rasterio (for the relief of the review map)
import json
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
MAPS = os.path.join(ROOT, "raw", "topotijdreis")
HOUSES = os.path.join(ROOT, "data", "houses.json")
LANDMARKS = os.path.join(ROOT, "data", "landmarks.json")
CORRECTIONS = os.path.join(ROOT, "data", "old_park_corrections.json")
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
SAME = 12.0            # two dots of different maps within this are one building [m]
SAME_1965 = 14.0       # and this for the map of 1965, which is less exact [m]
NAMED_WITHIN = 10.0    # a name of the corrections belongs to the nearest building within this [m]
PARK_NORTH = 575000.0  # north of this line stands the north-east group, south of it the park round the field;
NORTH_EAST = (118990.0, 574960.0)   # and so does what lies east and north of this corner


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


def reliefMap(buildings, others, houses, tower):
    """The buildings of the old park on the relief of today, with the houses of today"""
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
    colour[missing] = (0.78, 0.78, 0.78)
    zoom = 2
    image = Image.fromarray((np.clip(colour, 0.0, 1.0) * 255).astype(np.uint8)).resize((z.shape[1] * zoom, z.shape[0] * zoom))
    draw = ImageDraw.Draw(image)
    try:
        bold = ImageFont.truetype("DejaVuSans-Bold.ttf", 14)
        plain = ImageFont.truetype("DejaVuSans.ttf", 12)
    except OSError:
        bold = plain = ImageFont.load_default()
    at = lambda x, y: ((x - x0) * 2.0 * zoom, (y1 - y) * 2.0 * zoom)
    for number, x, y in houses:
        if x0 < x < x1 and y0 < y < y1:
            px, py = at(x, y)
            draw.text((px - 8, py + 10), str(number), fill=(90, 90, 90), font=plain)
    for x, y in others:
        px, py = at(x, y)
        draw.ellipse([px - 14, py - 14, px + 14, py + 14], outline=(0, 0, 0), width=2)
    for b in buildings:
        px, py = at(b["x"], b["y"])
        fill = {"Red": (205, 60, 40), "Green": (120, 140, 40), "Yellow": (225, 190, 60)}.get(b["door"], (150, 150, 150))
        if b["kind"] == "Chalet":
            fill = (90, 120, 190)
        draw.ellipse([px - 9, py - 9, px + 9, py + 9], fill=fill, outline=(255, 255, 255), width=2)
        draw.text((px + 12, py - 18), b["id"] if b["name"] == UNKNOWN else b["name"], fill=(60, 0, 0), font=bold)
    px, py = at(*tower)
    draw.rectangle([px - 9, py - 9, px + 9, py + 9], outline=(0, 0, 0), width=3)
    draw.text((px + 12, py - 7), "tower", fill=(0, 0, 0), font=bold)
    draw.rectangle([8, 8, 860, 62], fill=(255, 255, 255))
    draw.text((14, 12), "filled dot: building of the old park, from the map of 1983 (red, green, yellow: door known; blue: chalet; grey: door unknown)", fill=(0, 0, 0), font=plain)
    draw.text((14, 34), "open black ring: a dot found on the map of 1994, as a check   grey numbers: houses of today, on the relief of today (AHN)", fill=(0, 0, 0), font=plain)
    image.save(os.path.join(REVIEW, "old_park_on_the_relief_of_today.png"))


def main():
    os.makedirs(REVIEW, exist_ok=True)
    today = [h for h in json.load(open(HOUSES))["houses"] if h["kind"] == "building"]
    houses = np.array([[h["x"], h["y"]] for h in today])
    numbered = [(h["nr"], h["x"], h["y"]) for h in today if h["nr"]]
    corrections = json.load(open(CORRECTIONS))

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
        print(report[-1])

    master = found[MASTER]
    # The map of 1965 lies a little off: take out its median shift against the master
    distance = np.linalg.norm(found[1965][:, None] - master[None], axis=2)
    pairs = distance.min(1) < 20.0
    shift = np.median(master[distance.argmin(1)[pairs]] - found[1965][pairs], axis=0)
    found[1965] = found[1965] + shift
    print("1965 shifted by (%.1f, %.1f) m onto %d" % (shift[0], shift[1], MASTER))

    points = [(x, y, MASTER, "") for x, y in master]
    points += [(h["x"], h["y"], h["map"], h["why"]) for h in corrections["hidden"]]
    points.sort(key=lambda p: (-round(p[1] / 40.0), p[0]))      # in rows from the north, each from the west
    near = lambda cloud, x, y, within: bool(np.hypot(cloud[:, 0] - x, cloud[:, 1] - y).min() < within)
    buildings = []
    for k, (x, y, source, why) in enumerate(points):
        apart = np.hypot(houses[:, 0] - x, houses[:, 1] - y)
        nearest = today[int(apart.argmin())]
        buildings.append({
            "id": "OP%02d" % (k + 1), "x": round(float(x), 1), "y": round(float(y), 1),
            "group": "north-east" if y > PARK_NORTH or (x > NORTH_EAST[0] and y > NORTH_EAST[1]) else "park",
            "number": UNKNOWN, "name": UNKNOWN, "kind": "Bungalow", "door": UNKNOWN, "front_azimuth": UNKNOWN,
            "source": {"map": source, "read": "by eye" if why else "detected", "why": why,
                       "on_1994": near(found[1994], x, y, SAME), "on_1965": near(found[1965], x, y, SAME_1965)},
            "today": {"house": nearest["nr"] or nearest["type"], "distance": round(float(apart.min()), 1)}})
    unplaced = []
    for entry in corrections["named"]:
        apart = [np.hypot(b["x"] - entry["x"], b["y"] - entry["y"]) for b in buildings]
        k = int(np.argmin(apart))
        if apart[k] > NAMED_WITHIN:
            unplaced.append(entry)
            continue
        b = buildings[k]
        for key in ("name", "number", "door", "kind", "front_azimuth"):
            if entry.get(key):
                b[key] = entry[key]
        if entry.get("front_from_today"):
            standing = today[int(np.argmin(np.hypot(houses[:, 0] - b["x"], houses[:, 1] - b["y"])))]
            b["front_azimuth"] = standing["front_azimuth"]
        b["evidence"] = (b.get("evidence", "") + " " + entry["why"]).strip()
    for b in buildings:
        group = corrections["groups"].get(b["group"])
        if group:
            b["kind"] = group["kind"]
            b["since"] = group["since"]
            b["evidence"] = (b.get("evidence", "") + " " + group["why"]).strip()
    if unplaced:
        raise SystemExit("No building within %.0f m of: %s" % (NAMED_WITHIN, [(e["x"], e["y"]) for e in unplaced]))

    tower = json.load(open(LANDMARKS))["lookout_tower"]["rd"]
    out = {
        "note": "The buildings of the park as it was from 1957 to 1996, read off the topographic maps of "
                "raw/topotijdreis by tools/make_old_park.py; positions in RD New (EPSG:28992), metres, good to about "
                "5 m. id: ours, in rows from the north. number: the number the park gave it. name. kind: Bungalow, "
                "Chalet or Reception. door: Red, Green or Yellow, which is also its size (red the smallest). "
                "front_azimuth: the way the front door faces, degrees clockwise from north. What is not known is "
                "'Unknown'. group: park (round the field) or north-east. today: the nearest house of today.",
        "source": "Topotijdreis, Kadaster: TOP25 of %s" % ", ".join(str(y) for y in sorted(PANELS)),
        "master": MASTER, "report": report,
        "buildings": buildings}
    with open(OUT, "w") as f:
        json.dump(out, f, indent=1, ensure_ascii=False)
        f.write("\n")

    park = [b for b in buildings if b["group"] == "park"]
    lines = ["# The old park, 1957 to 1996", "", "Made by tools/make_old_park.py from raw/topotijdreis.", ""] + ["- " + r for r in report] + [
        "", "%d buildings: %d in the park, %d in the north-east group; %d of them within 12 m of a house of today."
        % (len(buildings), len(park), len(buildings) - len(park), sum(b["today"]["distance"] <= 12.0 for b in buildings)),
        "", "| id | number | name | kind | door | front | x | y | group | map | 1994 | 1965 | house today | m |", "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for b in buildings:
        s = b["source"]
        lines.append("| %s | %s | %s | %s | %s | %s | %.0f | %.0f | %s | %d%s | %s | %s | %s | %.0f |" % (
            b["id"], b["number"], b["name"], b["kind"], b["door"], b["front_azimuth"], b["x"], b["y"], b["group"], s["map"],
            " (by eye)" if s["why"] else "",
            "yes" if s["on_1994"] else "", "yes" if s["on_1965"] else "", b["today"]["house"], b["today"]["distance"]))
    with open(os.path.join(REVIEW, "old_park.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    reliefMap(buildings, found[1994][(found[1994][:, 1] < 575320.0) & (found[1994][:, 0] > 118540.0)], numbered, tower)
    print("%d buildings -> %s; review in %s" % (len(buildings), os.path.relpath(OUT, ROOT), os.path.relpath(REVIEW, ROOT)))


if __name__ == "__main__":
    main()
