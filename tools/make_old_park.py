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
#   5. fits the map of the park's own brochure (raw/park_brochures, read by hand into
#      data/old_park_brochures.json) onto them, by a linear fit that pairs each numbered bungalow with a dot;
#   6. decides. The park map says which bungalows there were, their number and their type; the topographic maps
#      say where: a bungalow stands on the dot it is paired with. One without a dot of 1983 takes a dot of 1994
#      or 1965 near it, and failing that its place on the park map. Dots that no bungalow takes are not
#      bungalows: the old reception, Krimweg 73, buildings of the camping.
#   7. lays the roads of the park map (data/old_park_brochures.json) on the roads of today (raw/bgt) wherever
#      one lies within 8 m, and leaves them where the park map has them elsewhere, and wherever the road of
#      today runs over an old bungalow;
#   8. adds what data/old_park_corrections.json knows besides: names, fronts, what kind each type is.
# Writes data/old_park.json and, to check it by, review/old_park/: the dots found on each map, everything on the
# relief of today (every map's idea of every bungalow, and the place decided on), and a table.
#
# How good: the two houses that still stand lie 2 m and 4 m from their dot of 1983. The dots of 1994 lie 4 m
# (median) from those of 1983; those of 1965 about 11 m, after a shift of 7 m is taken out, and are hard to tell
# from the red shading of the dunes. The park map, though drawn by hand, lies 4 m (median) from the dots after
# its fit; 77 of its 81 bungalows have a dot of 1983 within 15 m.
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
BGT = os.path.join(ROOT, "raw", "bgt")
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
ROAD_NEAR = 8.0        # an old road this near a road of today lies on it [m] (Jan de Kort, 2026-11-21)
ROAD_WIDEST = 12.0     # what is wider than this is a car park or a crossing, not a road to take the middle of [m]
ROAD_STEP = 2.0        # the old roads are worked out point by point, this far apart [m]
ROAD_STEADY = 30.0     # the move onto the road of today is the median of the moves over this length [m]
ROAD_SMOOTH = 12.0     # and the road is smoothed over this length [m]
ROAD_EASE = 30.0       # a road moved by a correction eases back to where it lay over this length [m]
ROAD_JOIN = 10.0       # an end this near another old road is led up to it [m]
ROAD_WIDTHS = {"Ring": 5.0, "Road": 3.5, "Path": 2.0}    # how wide each kind is taken to be [m]
BUNGALOW_CLEAR = 7.0   # a road of today this near the middle of an old bungalow runs over it: the park map decides [m]
BUNGALOW_REACH = 14.0  # and does so this far along the road to either side [m]
BUNGALOW_HALF = 6.0    # no old road comes nearer than this to the middle of a bungalow it passes [m]
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


def pairWithDots(parkMap, start, dots, tower):
    """Fits the park map onto the dots; returns the fit and, per number, the index of its dot"""
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


def reliefBase(x0=118540.0, x1=119180.0, y0=574620.0, y1=575320.0):
    """The relief of today, pale, to draw on: the picture, what to draw with, two fonts, RD -> pixel, and what lies on it"""
    import rasterio
    from rasterio.windows import from_bounds
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
    return image, draw, bold, plain, at, inside


def todaysRoads():
    """The roads of today (BGT), as one shape in RD"""
    from shapely.geometry import shape
    from shapely.ops import unary_union
    parts = []
    record = json.load(open(os.path.join(BGT, "wegdeel.json")))
    for page in record["pages"]:
        for feature in json.load(open(os.path.join(BGT, page["file"])))["features"]:
            if feature["properties"].get("eind_registratie") is None:
                parts.append(shape(feature["geometry"]).buffer(0))
    return unary_union(parts)


def alongLine(points, step):
    """Points every step along a line through the given points [m]"""
    points = np.asarray(points, float)
    lengths = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(points, axis=0), axis=1))]
    at = np.linspace(0.0, lengths[-1], max(2, int(round(lengths[-1] / step)) + 1))
    return np.c_[np.interp(at, lengths, points[:, 0]), np.interp(at, lengths, points[:, 1])]


def ontoTodaysRoad(points, roads):
    """Per point of a line: the middle of the road of today that lies within ROAD_NEAR of it, or the point itself.
    Returns the points and which of them were moved"""
    from shapely.geometry import LineString, Point
    from shapely.ops import nearest_points
    out = points.copy()
    moved = np.zeros(len(points), bool)
    for k, p in enumerate(points):
        spot = Point(p)
        if roads.distance(spot) > ROAD_NEAR:
            continue
        ahead = points[min(k + 1, len(points) - 1)] - points[max(k - 1, 0)]
        across = np.array([-ahead[1], ahead[0]]) / max(1.0e-9, np.linalg.norm(ahead))
        q = p if roads.contains(spot) else np.array(nearest_points(roads, spot)[0].coords[0])
        if not roads.contains(spot):
            across = (q - p) / max(1.0e-9, np.linalg.norm(q - p))
        chord = LineString([q - ROAD_WIDEST * across, q + ROAD_WIDEST * across]).intersection(roads)
        pieces = [g for g in getattr(chord, "geoms", [chord]) if g.geom_type == "LineString"]
        if not pieces:
            continue
        piece = min(pieces, key=lambda g: g.distance(Point(q)))
        middle = np.array(piece.interpolate(0.5, normalized=True).coords[0])
        if piece.length < ROAD_WIDEST and np.linalg.norm(middle - p) <= ROAD_NEAR + 0.5 * piece.length:
            out[k], moved[k] = middle, True        # a road: its middle
        # else a car park or a crossing, wider than a road: no middle to speak of, the point stays
    return out, moved


def oldRoads(traced, fit, roads, buildings, overrides):
    """The roads of the park map in RD: on the road of today where there is one within ROAD_NEAR, else where the
    park map has them; and where the park map has them wherever the road of today runs over an old bungalow"""
    homes = np.array([[b["x"], b["y"]] for b in buildings])
    out = []
    for road in traced:
        drawn = alongLine(fit(road["px"]), ROAD_STEP)
        line, moved = ontoTodaysRoad(drawn, roads)
        # Where a drive or a car park joins it, the middle of the road of today leaps aside: the move of each
        # point is the median of the moves round it
        shift = line - drawn
        half = int(round(0.5 * ROAD_STEADY / ROAD_STEP))
        line = drawn + np.array([np.median(shift[max(0, k - half):k + half + 1], axis=0) for k in range(len(shift))])
        # The exception: over a bungalow the road of today was laid later; the park map decides there
        over = np.linalg.norm(line[:, None] - homes[None], axis=2).min(1) < BUNGALOW_CLEAR
        over &= moved
        reach = int(round(BUNGALOW_REACH / ROAD_STEP))
        for k in np.flatnonzero(over):
            lo, hi = max(0, k - reach), min(len(line), k + reach + 1)
            line[lo:hi], moved[lo:hi] = drawn[lo:hi], False
        # No road runs through a bungalow: what the park map has within one is moved out of it
        for _ in range(3):
            apart = line[:, None] - homes[None]
            far = np.linalg.norm(apart, axis=2)
            nearest = far.argmin(1)
            for k in np.flatnonzero(far.min(1) < BUNGALOW_HALF):
                if 0 < k < len(line) - 1:        # an end may lead up to the door
                    away = apart[k, nearest[k]] / max(1.0e-9, far[k, nearest[k]])
                    line[k] = homes[nearest[k]] + BUNGALOW_HALF * away
        # What a photograph shows better than either map (data/old_park_corrections.json): the road is moved onto
        # the points given, and eases back to where it lay before the first and beyond the last
        for override in overrides:
            if override["name"] != road["name"]:
                continue
            through = np.array(override["through"], float)
            where = [int(np.linalg.norm(line - p, axis=1).argmin()) for p in through]
            shifts = [p - line[k] for p, k in zip(through, where)]
            reach = int(round(ROAD_EASE / ROAD_STEP))
            where = [where[0] - reach] + where + [where[-1] + reach]
            shifts = [np.zeros(2)] + shifts + [np.zeros(2)]
            at = np.arange(len(line))
            line = line + np.c_[np.interp(at, where, [s[0] for s in shifts]), np.interp(at, where, [s[1] for s in shifts])]
            road = dict(road, overridden=True)
        # Smooth, the ends kept
        smooth = line.copy()
        half = int(round(0.5 * ROAD_SMOOTH / ROAD_STEP))
        for k in range(1, len(line) - 1):
            reach = min(half, k, len(line) - 1 - k)
            smooth[k] = line[k - reach:k + reach + 1].mean(0)
        out.append({"name": road["name"], "kind": road["kind"], "width": ROAD_WIDTHS[road["kind"]],
                    "on_road_of_today": round(float(moved.mean()), 2), "over_bungalow": bool(over.any()),
                    "overridden": bool(road.get("overridden", False)), "drawn": drawn, "line": smooth})
    # An end that stops short of another road is led up to it
    for road in out:
        for end in (0, -1):
            best = None
            for other in out:
                if other is road and road["name"] != "ring road":
                    continue
                reach = other["line"] if other is not road else other["line"][5:-5]
                far = np.linalg.norm(reach - road["line"][end], axis=1)
                if far.min() < ROAD_JOIN and (best is None or far.min() < best[0]):
                    best = (far.min(), reach[far.argmin()])
            if best is not None and best[0] > 0.1:
                road["line"] = np.r_[[best[1]], road["line"]] if end == 0 else np.r_[road["line"], [best[1]]]
    return out


def roadMap(roads, todays, buildings, tower):
    """The old roads on the relief of today: as the park map has them, and as decided"""
    image, draw, bold, plain, at, inside = reliefBase(118420.0, 119180.0, 574620.0, 575480.0)
    mask = Image.new("L", image.size, 0)
    pen = ImageDraw.Draw(mask)
    for part in getattr(todays, "geoms", [todays]):
        pen.polygon([at(x, y) for x, y in part.exterior.coords], fill=255)
        for hole in part.interiors:
            pen.polygon([at(x, y) for x, y in hole.coords], fill=0)
    image.paste((165, 165, 172), mask=mask)
    for road in roads:
        draw.line([at(x, y) for x, y in road["drawn"]], fill=(0, 110, 255), width=2)
    for road in roads:
        draw.line([at(x, y) for x, y in road["line"]], fill=(150, 80, 20), width=int(round(road["width"] * 6.0)), joint="curve")
    for b in buildings:
        px, py = at(b["x"], b["y"])
        draw.ellipse([px - 9, py - 9, px + 9, py + 9], fill=(255, 255, 255), outline=(0, 0, 0), width=2)
        draw.text((px + 11, py - 20), str(b["number"]), fill=(0, 0, 0), font=bold)
    px, py = at(*tower)
    draw.rectangle([px - 9, py - 9, px + 9, py + 9], outline=(0, 0, 0), width=3)
    legend = ["grey: the roads of today (BGT)    blue line: the road as the park map has it, fitted",
              "brown: the old road as decided: on the road of today where that lies within %.0f m, else on the blue line" % ROAD_NEAR,
              "white dot with number: the bungalows of the old park"]
    draw.rectangle([8, 8, 900, 20 + 24 * len(legend)], fill=(255, 255, 255), outline=(0, 0, 0))
    for k, line in enumerate(legend):
        draw.text((16, 14 + 24 * k), line, fill=(0, 0, 0), font=plain)
    image.save(os.path.join(REVIEW, "old_park_roads.png"))


def reliefMap(buildings, found, parkPlace, unused, houses, tower):
    """Everything on the relief of today: every map's idea of every bungalow, and the place decided on"""
    image, draw, bold, plain, at, inside = reliefBase()
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
    for b in buildings:
        px, py = at(b["x"], b["y"])
        place = parkPlace.get(b["id"])
        if place is not None and inside(*place):
            qx, qy = at(*place)
            draw.line([px, py, qx, qy], fill=(0, 110, 255), width=2)
            draw.polygon([qx, qy - 7, qx - 6, qy + 5, qx + 6, qy + 5], fill=(0, 110, 255))
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
        "PARK MAP, fitted   blue triangle, tied to the place decided on",
        "DECIDED   thick ring with the number: red A4, green A6, yellow B4, blue B6",
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

    # The park map: every bungalow with its dot
    fit, paired = pairWithDots(brochures, brochures["start"], dots, tower)
    byNumber = {b["number"]: b for b in brochures["bungalows"]}
    parkPlace = {key: fit([[b["x_px"], b["y_px"]]])[0] for key, b in byNumber.items()}
    place, source = {}, {}
    for key in parkPlace:
        if key in paired:
            place[key], source[key] = dots[paired[key]], dotSource[paired[key]]
    taken = set(paired.values())
    for key in sorted(k for k in parkPlace if k not in place):
        spot = parkPlace[key]
        for year in (1994, 1965):
            away = np.linalg.norm(found[year] - spot, axis=1)
            if away.min() < OTHER_MAP:
                place[key], source[key] = found[year][int(away.argmin())], "map of %d" % year
                break
        else:
            place[key], source[key] = spot, "park map"
    off = [np.linalg.norm(parkPlace[key] - dots[paired[key]]) for key in paired]
    report.append("the park map: %d of its %d bungalows with a dot of %d, median %.1f m from it, at most %.1f m"
                  % (len(off), len(byNumber), MASTER, np.median(off), max(off)))
    unused = [dots[j] for j in range(len(dots)) if j not in taken]

    order = sorted(place)
    buildings = []
    notes = []
    for key in order:
        e = byNumber[key]
        kind = e["type"]
        x, y = place[key]
        apart = np.hypot(houses[:, 0] - x, houses[:, 1] - y)
        nearest = today[int(apart.argmin())]
        what = corrections["types"][kind]
        b = {"id": "OP%s" % key, "number": e["number"], "name": UNKNOWN, "kind": what["kind"], "type": kind,
             "front_azimuth": UNKNOWN, "x": round(float(x), 1), "y": round(float(y), 1)}
        if "since" in what:
            b["since"] = what["since"]
        b["source"] = {"position": source[key], "park_map_off": round(float(np.linalg.norm(parkPlace[key] - place[key])), 1)}
        b["today"] = {"house": nearest["nr"] or nearest["type"], "distance": round(float(apart.min()), 1)}
        buildings.append(b)
    known = {b["number"]: b for b in buildings}
    for entry in corrections["named"]:
        b = known[entry["number"]]
        for field in ("name", "front_azimuth"):
            if field in entry:
                b[field] = entry[field]
        if "position" in entry:
            # A photograph, or the park map where the dot of the topographic map lies on a slope, knows better
            spot = parkPlace[entry["number"]] if entry["position"] == "park map" else np.array(entry["position"], float)
            moved = float(np.hypot(spot[0] - b["x"], spot[1] - b["y"]))
            b["x"], b["y"] = round(float(spot[0]), 1), round(float(spot[1]), 1)
            b["source"]["position"] = ("park map" if entry["position"] == "park map" else "photograph") \
                + ", %.0f m from the %s" % (moved, b["source"]["position"])
            b["source"]["park_map_off"] = round(float(np.linalg.norm(parkPlace[entry["number"]] - spot)), 1)
            apart = np.hypot(houses[:, 0] - b["x"], houses[:, 1] - b["y"])
            nearest = today[int(apart.argmin())]
            b["today"] = {"house": nearest["nr"] or nearest["type"], "distance": round(float(apart.min()), 1)}
        if entry.get("front_from_today"):
            standing = today[int(np.argmin(np.hypot(houses[:, 0] - b["x"], houses[:, 1] - b["y"])))]
            b["front_azimuth"] = standing["front_azimuth"]
        if "door_seen" in entry and entry["door_seen"] != doors[b["type"]]:
            notes.append("%s: its door is %s in a photograph, but type %s has a %s door"
                         % (entry["number"], entry["door_seen"], b["type"], doors[b["type"]]))
        b["evidence"] = entry["why"]

    others = []
    for other in brochures["others"]:
        left, right = fit(other["front_px"])
        along = (right - left) / np.linalg.norm(right - left)
        outward = np.array([along[1], -along[0]])       # a quarter turn clockwise of left to right
        middle = 0.5 * (left + right) - 0.5 * other["depth"] * outward
        others.append({"name": other["name"], "type": other["type"], "x": round(float(middle[0]), 1), "y": round(float(middle[1]), 1),
                       "front_azimuth": round(float(np.degrees(np.arctan2(outward[0], outward[1])) % 360.0), 1),
                       "front_length": round(float(np.linalg.norm(right - left)), 1), "evidence": other["why"]})
        report.append("%s: its front is %.1f m long on the park map and faces %.0f degrees"
                      % (other["name"], others[-1]["front_length"], others[-1]["front_azimuth"]))
    todays = todaysRoads()
    roads = oldRoads(brochures["roads"], fit, todays, buildings, corrections.get("roads", []))
    report.append("%d old roads, %.0f m: %.0f%% on a road of today; where a road of today runs over a bungalow the park map decides: %s"
                  % (len(roads), sum(ROAD_STEP * (len(r["line"]) - 1) for r in roads),
                     100.0 * np.mean(np.concatenate([[r["on_road_of_today"]] * len(r["line"]) for r in roads])),
                     ", ".join(r["name"] for r in roads if r["over_bungalow"]) or "nowhere"))
    out = {
        "note": "The buildings of the park as it was from 1957 to 1996. Which there were, their number and their type "
                "(A4, A6, B4, B6) are those of the park's own map (raw/park_brochures); their positions those "
                "of the topographic maps (raw/topotijdreis) where these have them, RD New (EPSG:28992), metres, good to "
                "about 5 m. Made by tools/make_old_park.py. The door, and so the model, follows from the type in "
                "data/type_models.json. kind: Bungalow or Chalet. front_azimuth: the way the front door faces, degrees "
                "clockwise from north. What is not known is 'Unknown'. source.position: where the position is from; "
                "park_map_off: how far the park map, fitted, has it from there [m]. today: the "
                "nearest house of today. others: buildings that are not bungalows: the middle of the part under the main roof, the "
                "way the front faces, and the length of the front on the park map. roads: the roads of the old park, all unpaved, from the park map: kind Ring, Road or "
                "Path, width [m], the line through its middle, on_road_of_today: the part of it that was laid on a road "
                "of today, over_bungalow: whether a road of today that runs over an old bungalow was left for the park map, overridden: "
                "whether data/old_park_corrections.json moves it.",
        "source": "Map of the park's brochure; Topotijdreis, Kadaster: TOP25 of %s" % ", ".join(str(y) for y in sorted(PANELS)),
        "report": report, "notes": notes,
        "others": others,
        "roads": [{"name": r["name"], "kind": r["kind"], "width": r["width"], "on_road_of_today": r["on_road_of_today"],
                   "over_bungalow": r["over_bungalow"], "overridden": r["overridden"], "line": [[round(float(x), 1), round(float(y), 1)] for x, y in r["line"]]}
                  for r in roads],
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
        "", "| number | name | type | kind | front | x | y | position from | park map off | house today | m |",
        "|---|---|---|---|---|---|---|---|---|---|---|"]
    for b in buildings:
        lines.append("| %s | %s | %s | %s | %s | %.0f | %.0f | %s | %.0f | %s | %.0f |" % (
            b["number"], "" if b["name"] == UNKNOWN else b["name"], b["type"], b["kind"], b["front_azimuth"], b["x"], b["y"],
            b["source"]["position"], b["source"]["park_map_off"], b["today"]["house"], b["today"]["distance"]))
    with open(os.path.join(REVIEW, "old_park.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    roadMap(roads, todays, buildings, tower)
    reliefMap(buildings, found, {"OP%s" % key: value for key, value in parkPlace.items()}, unused, numbered, tower)
    for line in report + notes:
        print(line)
    print("%d buildings -> %s; review in %s" % (len(buildings), os.path.relpath(OUT, ROOT), os.path.relpath(REVIEW, ROOT)))


if __name__ == "__main__":
    main()
