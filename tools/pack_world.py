#!/usr/bin/env python3
# Slufter3D - packs the world for the application: docs/data/world.bin
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/pack_world.py      (after tools/make_terrain.py)
#
# The application reads one binary file when it starts; nothing of the world is compiled into it.
# This script gathers that file from the interpretation layers:
#   build/terrain/      the ground of tools/make_terrain.py   -> three grids, coarser further out
#   data/houses.json    houses and pitches                    -> with their model from data/type_models.json
#   data/landmarks.json start, lookout tower, entrance, lighthouse
#   raw/bgt/            road parts as they are now            -> cut along the triangles of the ground
# The layout of the file is that of the write() calls at the end; src/WorldData.cpp reads it in the same order.
# A preview of the roads goes to build/preview/world/roads.png.
#
# Needs: numpy, scipy, rasterio, shapely (2.1 or later)
import json
import math
import os
import struct

import numpy as np
import rasterio
import shapely
from scipy import ndimage
from shapely.geometry import LineString, MultiPoint, Point, Polygon, box, shape
from shapely.prepared import prep

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
TERRAIN = os.path.join(ROOT, "build", "terrain")
OUT = os.path.join(ROOT, "docs", "data", "world.bin")
PREVIEW = os.path.join(ROOT, "build", "preview", "world")

VERSION = 5              # 5: every road triangle has its eras; 4: every house has the eras it stands in; 3: the water of a tile may be SEA_MARK
TREES = os.path.join(ROOT, "data", "trees.json")   # made by tools/make_trees.py; without it: no trees
ORIGIN = (118700.0, 574700.0)   # RD New of the origin of the application's coordinates: east and north in metres

# The grids, finest first: source of tools/make_terrain.py, cell [m], and west, south, east, north in metres
# from ORIGIN. Each lies within its source raster, and its edges lie on the lines of the next, coarser grid.
LEVELS = [
    {"name": "park", "cell": 3.0, "extent": (-288.0, -60.0, 552.0, 792.0)},
    {"name": "tip", "cell": 12.0, "extent": (-4200.0, -300.0, 1200.0, 3300.0)},
    {"name": "horizon", "cell": 60.0, "extent": (-5700.0, -4200.0, 4260.0, 5280.0)},
]
SEAM_ROWS = 4            # a grid eases into the next one over this many of its rows from the edge
WATER_SHARE = 0.4        # a tile is water when this part of it is
SEA_SHARE = 0.5          # a tile is sea when this part of it is, in the classes of tools/make_terrain.py
NO_WATER = -32768
SEA_MARK = -32767        # in place of a water level: the tile is sea, which lies at 0 m NAP
WATER, SEA = 2, 3        # classes of tools/make_terrain.py

# Paving of a BGT road part (fysiek_voorkomen) -> kind in the file
PAVING = {"gesloten verharding": 0, "open verharding": 1, "half verhard": 2, "onverhard": 3}
PAVING_NAMES = ["asphalt", "bricks", "shells", "sand"]
OLD_ROAD = 3             # every road of the old park was unpaved (Jan de Kort, 2026-11-21)
PARK_HULL = 0.2          # how closely the grounds of the park follow their outermost houses (shapely.concave_hull)
PARK_MARGIN = 12.0       # and how far they reach beyond them [m]

# The central building is covered with rectangular wings, each under its own roof
WING_CELL = 0.5          # raster used to find the wings [m]
WING_MIN = 4.0           # narrowest wing [m]
WING_MAX = 11.0          # widest roof; wider parts get several roofs side by side [m]


# ---------------------------------------------------------------- the ground

def readTerrain(name):
    with rasterio.open(os.path.join(TERRAIN, name + "_ground.tif")) as source:
        ground = source.read(1)
        level = source.read(2)
        transform = source.transform
    with rasterio.open(os.path.join(TERRAIN, name + "_class.tif")) as source:
        classes = source.read(1)
    return ground, level, classes, transform


def makeLevel(spec):
    """Heights at the nodes [m NAP] and the water of every tile, rows from south to north: its level in cm NAP,
    NO_WATER when it is dry, SEA_MARK when it is sea"""
    ground, waterLevel, classes, transform = readTerrain(spec["name"])
    cell = spec["cell"]
    west, south, east, north = spec["extent"]
    nx = int(round((east - west) / cell)) + 1
    nz = int(round((north - south) / cell)) + 1
    size = int(round(cell / transform.a))
    if abs(size * transform.a - cell) > 1e-9:
        raise RuntimeError("The cell of %s is not a whole number of cells of its raster" % spec["name"])
    # A node takes the mean of the ground one cell around it
    mean = ndimage.uniform_filter(ground.astype(np.float64), size=size, mode="nearest")
    x = ORIGIN[0] + west + np.arange(nx) * cell
    y = ORIGIN[1] + south + np.arange(nz) * cell
    cols = np.round((x - transform.c) / transform.a).astype(int)
    rows = np.round((y - transform.f) / transform.e).astype(int)
    if cols.min() < 0 or cols.max() > ground.shape[1] or rows.min() < 0 or rows.max() > ground.shape[0]:
        raise RuntimeError("The grid of %s reaches beyond its raster" % spec["name"])
    cols = np.clip(cols, 0, ground.shape[1] - 1)
    rows = np.clip(rows, 0, ground.shape[0] - 1)
    heights = mean[np.ix_(rows, cols)]

    # Water of a tile: the share of inland water in it, and its mean level
    wet = (classes == WATER).astype(np.float64)
    share = ndimage.uniform_filter(wet, size=size, mode="constant")
    levelSum = ndimage.uniform_filter(np.where(classes == WATER, waterLevel, 0.0).astype(np.float64), size=size, mode="constant")
    tileCols = np.clip(cols[:-1] + size // 2, 0, ground.shape[1] - 1)
    tileRows = np.clip(rows[:-1] - size // 2, 0, ground.shape[0] - 1)
    tileShare = share[np.ix_(tileRows, tileCols)]
    tileLevel = levelSum[np.ix_(tileRows, tileCols)] / np.maximum(tileShare, 1e-9)
    water = np.where(tileShare >= WATER_SHARE, np.round(tileLevel * 100.0), NO_WATER)
    # The sea, as tools/make_terrain.py found it: the application takes nothing else for sea
    seaShare = ndimage.uniform_filter((classes == SEA).astype(np.float64), size=size, mode="constant")
    water = np.where(seaShare[np.ix_(tileRows, tileCols)] >= SEA_SHARE, SEA_MARK, water).astype(np.int16)
    return {"name": spec["name"], "cell": cell, "west": west, "south": south, "nx": nx, "nz": nz,
            "heights": heights, "water": water}


def heightOn(level, east, north):
    """Height of the triangles of a grid at points: each tile is split from its south-west to its north-east corner"""
    u = np.clip((np.asarray(east) - level["west"]) / level["cell"], 0.0, level["nx"] - 1.0)
    v = np.clip((np.asarray(north) - level["south"]) / level["cell"], 0.0, level["nz"] - 1.0)
    i = np.minimum(u.astype(int), level["nx"] - 2)
    j = np.minimum(v.astype(int), level["nz"] - 2)
    fu, fv = u - i, v - j
    h = level["heights"]
    h00, h10, h01, h11 = h[j, i], h[j, i + 1], h[j + 1, i], h[j + 1, i + 1]
    return np.where(fu >= fv, h00 + (h10 - h00) * fu + (h11 - h10) * fv, h00 + (h11 - h01) * fu + (h01 - h00) * fv)


def joinLevels(levels):
    """Eases every grid into the next, coarser one, so that their edges meet without a crack"""
    for fine, coarse in zip(levels[:-1], levels[1:]):
        east = fine["west"] + np.arange(fine["nx"]) * fine["cell"]
        north = fine["south"] + np.arange(fine["nz"]) * fine["cell"]
        ee, nn = np.meshgrid(east, north)
        fromEdge = np.minimum(np.minimum(np.arange(fine["nx"])[None, :], np.arange(fine["nx"])[None, ::-1]),
                              np.minimum(np.arange(fine["nz"])[:, None], np.arange(fine["nz"])[::-1, None]))
        weight = np.clip(1.0 - fromEdge / float(SEAM_ROWS), 0.0, 1.0)
        fine["heights"] = weight * heightOn(coarse, ee, nn) + (1.0 - weight) * fine["heights"]


# ---------------------------------------------------------------- houses

def packHouses(levels):
    types = json.load(open(os.path.join(ROOT, "data", "type_models.json")))["types"]
    floors = {(f["nr"], f["x"], f["y"]): f["floor"]
              for f in json.load(open(os.path.join(TERRAIN, "terrain.json"))).get("houses", [])}
    houses = []
    park = json.load(open(os.path.join(ROOT, "data", "houses.json")))
    for h in park["houses"]:
        entry = types[h["type"]]
        semi = bool(h.get("semi_with"))
        houses.append({"nr": h["nr"], "type": h["type"], "name": h["name"] if h["kind"] == "building" else "",
                       "model": entry.get("semi_model", entry["model"]) if semi else entry["model"],
                       "semi": semi, "semi_with": h.get("semi_with") or 0, "pitch": h["kind"] != "building", "outline": entry["size"] == "outline",
                       "east": h["x"] - ORIGIN[0], "north": h["y"] - ORIGIN[1], "length": h["length"], "width": h["width"],
                       "axis": h["axis_azimuth"], "front": h["front_azimuth"],
                       "floor": floors[(h["nr"], h["x"], h["y"])]})
        model = houses[-1]["model"]
        houses[-1]["eras"] = (ALWAYS if h["type"] in ALWAYS_THERE else TODAY if model.startswith(LUXE) else SINCE_1993)
    return houses + packOldPark(levels, types, houses)


# The eras of the park, as bits: 1->1957 to 1971, 2->1971 to 1993, 4->1993 to 2013, 8->2013 to today
OLD, OLD_WITH_CHALETS, COMFORT, TODAY = 1, 2, 4, 8
SINCE_1993 = COMFORT | TODAY
ALWAYS = OLD | OLD_WITH_CHALETS | COMFORT | TODAY
LUXE = ("Luxe", "Notched")              # the models of the Luxe bungalows start so: they came in 2013
STANDING = ("Visdiefje",)               # the type of the bungalows of the old park that still stand
ALWAYS_THERE = ("Visdiefje", "Krimweg73")   # types of houses that stood before 1993 and still stand
FALLBACK_FRONT = 270.0     # a front that is not known: the terrace, right of the front door, then faces south
SAME_BUILDING = 8.0        # an old building this near a house that still stands is that house [m]
OTHER_FOOT = 8.0           # and this far round the middle of the Taveerne [m]
OLD_DOOR = (-1.2, 3.1)     # where the front door of an old bungalow is: right of its middle, before it [m]
OLD_DUG = 0.6              # an old bungalow lies at most this deep in the higher ground round it [m]
OLD_FOOT = 3.0             # the ground is taken this far round the middle of an old building [m]
FLOOR_PERCENTILE = 75.0    # of that ground: on the high side of the plot, as tools/make_terrain.py has it
FLOOR_LIFT = 0.15          # and the floor this far above it [m]


def packOldPark(levels, types, houses):
    """The buildings of the park of 1957 to 1996 (data/old_park.json), on the ground of today"""
    standing = [h for h in houses if h["type"] in STANDING]
    old = []
    park = json.load(open(os.path.join(ROOT, "data", "old_park.json")))
    for b in park["buildings"]:
        east, north = b["x"] - ORIGIN[0], b["y"] - ORIGIN[1]
        name = "" if b["name"] == "Unknown" else b["name"]
        still = [h for h in standing if math.hypot(h["east"] - east, h["north"] - north) < SAME_BUILDING]
        if still:
            # It still stands: the house of today, where it stands today, under its old number and type
            old.append(dict(still[0], nr=b["number"], type=b["type"], name=name, eras=OLD | OLD_WITH_CHALETS))
            still[0]["eras"] = SINCE_1993
            continue
        chalet = b["kind"] == "Chalet"
        front = b["front_azimuth"] if b["front_azimuth"] != "Unknown" else FALLBACK_FRONT
        steps = np.linspace(-OLD_FOOT, OLD_FOOT, 5)
        ee, nn = np.meshgrid(east + steps, north + steps)
        high = float(np.percentile(heightOn(levels[0], ee, nn), FLOOR_PERCENTILE))
        # The floor lies a step above the ground at the front door, so that the doorstep is low; the dune may
        # stand against the other walls, but no higher than OLD_DUG
        f, a = math.radians(front), math.radians(front - 90.0)
        doorEast = east + OLD_DOOR[0] * math.sin(a) + OLD_DOOR[1] * math.sin(f)
        doorNorth = north + OLD_DOOR[0] * math.cos(a) + OLD_DOOR[1] * math.cos(f)
        atDoor = float(heightOn(levels[0], np.array([doorEast]), np.array([doorNorth]))[0])
        floor = max(atDoor + FLOOR_LIFT, high - OLD_DUG)
        old.append({"nr": b["number"], "type": b["type"], "name": name,
                    "model": types[b["type"]]["model"], "semi": False, "semi_with": 0, "pitch": False, "outline": False,
                    "east": east, "north": north, "length": 0.0, "width": 0.0, "axis": (front - 90.0) % 360.0,
                    "front": front, "floor": floor, "eras": OLD_WITH_CHALETS if chalet else OLD | OLD_WITH_CHALETS})
    for b in park["others"]:
        east, north = b["x"] - ORIGIN[0], b["y"] - ORIGIN[1]
        steps = np.linspace(-OTHER_FOOT, OTHER_FOOT, 9)
        ee, nn = np.meshgrid(east + steps, north + steps)
        floor = float(np.percentile(heightOn(levels[0], ee, nn), FLOOR_PERCENTILE)) + FLOOR_LIFT
        old.append({"nr": 0, "type": b["type"], "name": b["name"], "model": types[b["type"]]["model"], "semi": False,
                    "semi_with": 0, "pitch": False, "outline": False, "east": east, "north": north, "length": 0.0,
                    "width": 0.0, "axis": (b["front_azimuth"] - 90.0) % 360.0, "front": b["front_azimuth"], "floor": floor,
                    "eras": OLD | OLD_WITH_CHALETS})
    return old


def largestRectangle(free):
    """Largest all-true axis-parallel rectangle: (area, row0, row1, col0, col1), ends exclusive"""
    rows, cols = free.shape
    heights = np.zeros(cols, dtype=int)
    best = (0, 0, 0, 0, 0)
    for r in range(rows):
        heights = np.where(free[r], heights + 1, 0)
        stack = []
        for c in range(cols + 1):
            h = heights[c] if c < cols else 0
            start = c
            while stack and stack[-1][1] >= h:
                start, sh = stack.pop()
                area = sh * (c - start)
                if area > best[0]:
                    best = (area, r - sh + 1, r + 1, start, c)
            stack.append((start, h))
    return best


def makeWings(outline):
    """Rectangles along the main direction of the central building that cover its outline"""
    edges = list(outline.minimum_rotated_rectangle.exterior.coords)
    long = max(((edges[k + 1][0] - edges[k][0], edges[k + 1][1] - edges[k][1]) for k in range(2)), key=lambda e: math.hypot(*e))
    angle = math.atan2(long[1], long[0])              # of the main direction, from east towards north
    ca, sa = math.cos(angle), math.sin(angle)
    cx, cy = outline.centroid.x, outline.centroid.y
    turned = Polygon([((x - cx) * ca + (y - cy) * sa, -(x - cx) * sa + (y - cy) * ca) for x, y in outline.exterior.coords])
    u0, v0, u1, v1 = turned.bounds
    nu = int(math.ceil((u1 - u0) / WING_CELL))
    nv = int(math.ceil((v1 - v0) / WING_CELL))
    inside = prep(turned.buffer(0.3))
    mask = np.array([[inside.contains(Point(u0 + (i + 0.5) * WING_CELL, v0 + (j + 0.5) * WING_CELL)) for i in range(nu)]
                     for j in range(nv)])
    free = mask.copy()
    least = int(WING_MIN / WING_CELL)
    wings = []
    while True:
        area, r0, r1, c0, c1 = largestRectangle(free)
        if r1 - r0 < least or c1 - c0 < least:
            break
        free[r0:r1, c0:c1] = False
        grown = True      # into the rest of the building, so that roofs run into each other
        while grown:
            grown = False
            if r0 > 0 and mask[r0 - 1, c0:c1].all():
                r0 -= 1
                grown = True
            if r1 < nv and mask[r1, c0:c1].all():
                r1 += 1
                grown = True
            if c0 > 0 and mask[r0:r1, c0 - 1].all():
                c0 -= 1
                grown = True
            if c1 < nu and mask[r0:r1, c1].all():
                c1 += 1
                grown = True
        du, dv = (c1 - c0) * WING_CELL, (r1 - r0) * WING_CELL
        um, vm = u0 + 0.5 * (c0 + c1) * WING_CELL, v0 + 0.5 * (r0 + r1) * WING_CELL
        east, north = cx + um * ca - vm * sa, cy + um * sa + vm * ca
        along = angle if du >= dv else angle + 0.5 * math.pi
        azimuth = (90.0 - math.degrees(along)) % 180.0
        length, width = max(du, dv), min(du, dv)
        bays = int(math.ceil(width / WING_MAX))
        az = math.radians(azimuth)
        for k in range(bays):
            offset = ((k + 0.5) / bays - 0.5) * width      # across the long axis
            wings.append({"east": east + offset * math.cos(az) - ORIGIN[0], "north": north - offset * math.sin(az) - ORIGIN[1],
                          "length": length, "width": width / bays, "axis": azimuth})
    covered = 1.0 - free.sum() / float(mask.sum())
    return wings, covered


def entranceOnWall(wings, east, north):
    """The point on the outside wall of the wings nearest to a spot, and the way that wall faces [deg]"""
    best = None
    for wing in wings:
        a = math.radians(wing["axis"])
        along, across = (math.sin(a), math.cos(a)), (math.cos(a), -math.sin(a))
        du = (east - wing["east"]) * along[0] + (north - wing["north"]) * along[1]
        dv = (east - wing["east"]) * across[0] + (north - wing["north"]) * across[1]
        hl, hw = 0.5 * wing["length"], 0.5 * wing["width"]
        for u, v, facing in ((min(max(du, -hl), hl), hw, wing["axis"] + 90.0), (min(max(du, -hl), hl), -hw, wing["axis"] - 90.0),
                             (hl, min(max(dv, -hw), hw), wing["axis"]), (-hl, min(max(dv, -hw), hw), wing["axis"] + 180.0)):
            pe = wing["east"] + u * along[0] + v * across[0]
            pn = wing["north"] + u * along[1] + v * across[1]
            f = math.radians(facing)
            out = (pe + 0.3 * math.sin(f), pn + 0.3 * math.cos(f))
            inAnother = any(abs((out[0] - w["east"]) * math.sin(math.radians(w["axis"])) + (out[1] - w["north"]) * math.cos(math.radians(w["axis"]))) < 0.5 * w["length"]
                            and abs((out[0] - w["east"]) * math.cos(math.radians(w["axis"])) - (out[1] - w["north"]) * math.sin(math.radians(w["axis"]))) < 0.5 * w["width"]
                            for w in wings)
            distance = math.hypot(pe - east, pn - north)
            if not inAnother and (best is None or distance < best[0]):
                best = (distance, pe, pn, facing % 360.0)
    return best[1], best[2], best[3]


# ---------------------------------------------------------------- roads

def currentFeatures(collection):
    """The objects of a BGT collection as they are now: earlier versions have an end of registration"""
    record = json.load(open(os.path.join(ROOT, "raw", "bgt", collection + ".json")))
    for page in record["pages"]:
        for feature in json.load(open(os.path.join(ROOT, "raw", "bgt", page["file"])))["features"]:
            if feature["properties"].get("eind_registratie") is None:
                yield feature


def parkOf(houses, old):
    """The grounds of the park, roughly: round the houses of today and the buildings and roads of the old park"""
    points = [(h["east"], h["north"]) for h in houses if h["nr"] or h["pitch"]]
    points += [(b["x"] - ORIGIN[0], b["y"] - ORIGIN[1]) for b in old["buildings"]]
    points += [(x - ORIGIN[0], y - ORIGIN[1]) for road in old["roads"] for x, y in road["line"]]
    return shapely.concave_hull(MultiPoint(points), ratio=PARK_HULL).buffer(PARK_MARGIN)


def makeRoads(level, houses):
    """The road parts within the finest grid, cut along its triangles: every piece lies flat on the ground.
    The roads of today (BGT) within the park came with the Comfort bungalows; before 1993 the park had its own,
    unpaved (data/old_park.json). Roads outside the park are taken to have been there all along.
    Returns triangles (n, 6) as east, north of three corners, their kinds (n) and their eras (n)"""
    cell, west, south = level["cell"], level["west"], level["south"]
    east, north = west + (level["nx"] - 1) * cell, south + (level["nz"] - 1) * cell
    frame = box(west, south, east, north)
    old = json.load(open(os.path.join(ROOT, "data", "old_park.json")))
    park = parkOf(houses, old)
    parts = []      # shape, kind, eras, name to count it under
    for feature in currentFeatures("wegdeel"):
        kind = PAVING.get((feature["properties"].get("fysiek_voorkomen") or "").lower())
        if kind is None:
            continue
        road = shapely.transform(shape(feature["geometry"]), lambda p: p - np.array(ORIGIN)).buffer(0).intersection(frame)
        if road.is_empty or road.area < 0.01:
            continue
        inPark = road.intersection(park).area > 0.5 * road.area
        parts.append((road, kind, SINCE_1993 if inPark else ALWAYS, PAVING_NAMES[kind]))
    laid = [road for road, kind, when, name in parts if when == ALWAYS]     # an old road ends where a public one lies
    for road in sorted(old["roads"], key=lambda r: -r["width"]):
        line = LineString([(x - ORIGIN[0], y - ORIGIN[1]) for x, y in road["line"]])
        strip = line.buffer(0.5 * road["width"], cap_style="round", join_style="round").intersection(frame)
        for other in laid:
            strip = strip.difference(other)        # no two roads on one spot
        if not strip.is_empty and strip.area > 0.01:
            laid.append(strip)
            parts.append((strip, OLD_ROAD, OLD | OLD_WITH_CHALETS, "sand of the old park"))
    triangles, kinds, eras, counts = [], [], [], {}
    for road, kind, when, name in parts:
        counts[name] = counts.get(name, 0.0) + road.area
        ready = prep(road)
        w, s, e, n = road.bounds
        for j in range(max(int((s - south) / cell), 0), min(int((n - south) / cell) + 1, level["nz"] - 1)):
            for i in range(max(int((w - west) / cell), 0), min(int((e - west) / cell) + 1, level["nx"] - 1)):
                x0, y0 = west + i * cell, south + j * cell
                sw, se, ne, nw = (x0, y0), (x0 + cell, y0), (x0 + cell, y0 + cell), (x0, y0 + cell)
                for corners in ((sw, se, ne), (sw, ne, nw)):
                    tile = Polygon(corners)
                    if ready.contains(tile):
                        pieces = [tile]
                    elif ready.intersects(tile):
                        cut = road.intersection(tile)
                        polygons = [g for g in getattr(cut, "geoms", [cut]) if g.geom_type == "Polygon" and g.area > 1e-4]
                        pieces = [t for part in polygons for t in shapely.constrained_delaunay_triangles(part).geoms
                                  if t.area > 1e-5]
                    else:
                        continue
                    for piece in pieces:
                        c = list(piece.exterior.coords)[:3]
                        triangles.append([c[0][0], c[0][1], c[1][0], c[1][1], c[2][0], c[2][1]])
                        kinds.append(kind)
                        eras.append(when)
    return np.array(triangles, np.float32).reshape(-1, 6), np.array(kinds, np.uint8), np.array(eras, np.uint8), counts


def previewRoads(level, triangles, kinds, houses, name="roads.png", title="Roads of the BGT on the ground of the park."):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.collections import PolyCollection
    colours = np.array([[0.40, 0.40, 0.42], [0.62, 0.42, 0.34], [0.93, 0.88, 0.72], [0.80, 0.72, 0.52]])
    figure, axis = plt.subplots(figsize=(11.0, 11.0), dpi=150)
    extent = (level["west"], level["west"] + (level["nx"] - 1) * level["cell"],
              level["south"], level["south"] + (level["nz"] - 1) * level["cell"])
    axis.imshow(level["heights"], extent=extent, origin="lower", cmap="gist_earth", vmin=-6.0, vmax=16.0)
    axis.add_collection(PolyCollection(triangles.reshape(-1, 3, 2), facecolors=colours[kinds], edgecolors="none"))
    for h in houses:
        axis.text(h["east"], h["north"], str(h["nr"]) if h["nr"] else "", fontsize=3, ha="center", va="center", color="white")
    axis.set_title(title + " Grey asphalt, brown bricks, pale shells, sand. "
                   "Source: BGT, Kadaster (CC BY 4.0)", fontsize=9)
    axis.set_xlabel("east of the origin [m]")
    axis.set_ylabel("north of the origin [m]")
    os.makedirs(PREVIEW, exist_ok=True)
    out = os.path.join(PREVIEW, name)
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


# ---------------------------------------------------------------- the file

def text(value):
    raw = value.encode("utf-8")
    if len(raw) > 255:
        raise RuntimeError("Text too long for the file: %s" % value)
    return struct.pack("<B", len(raw)) + raw


def main():
    levels = [makeLevel(spec) for spec in LEVELS]
    joinLevels(levels)
    houses = packHouses(levels)
    data = json.load(open(os.path.join(ROOT, "data", "houses.json")))
    wings, covered = makeWings(Polygon(data["central_building"]["outline"]))
    marks = json.load(open(os.path.join(ROOT, "data", "landmarks.json")))

    def local(rd):
        return rd[0] - ORIGIN[0], rd[1] - ORIGIN[1]

    entrance = entranceOnWall(wings, *local(marks["reception_entrance"]["rd"]))
    triangles, kinds, roadEras, areas = makeRoads(levels[0], houses)

    out = bytearray()
    out += struct.pack("<4sI", b"SLF3", VERSION)
    out += struct.pack("<dd", *ORIGIN)
    out += struct.pack("<I", len(levels))
    for level in levels:
        out += struct.pack("<dddII", level["west"], level["south"], level["cell"], level["nx"], level["nz"])
        out += np.round(level["heights"] * 100.0).astype("<i2").tobytes()     # cm NAP, rows from south to north
        out += level["water"].astype("<i2").tobytes()                          # cm NAP per tile, NO_WATER, SEA_MARK
    out += struct.pack("<I", len(houses))
    for h in houses:
        out += struct.pack("<ii", h["nr"], h["semi_with"]) + text(h["type"]) + text(h["name"]) + text(h["model"])
        out += struct.pack("<BBBB", h["semi"], h["pitch"], h["outline"], h["eras"])
        out += struct.pack("<7f", h["east"], h["north"], h["length"], h["width"], h["axis"], h["front"], h["floor"])
    out += struct.pack("<I", len(wings))
    for wing in wings:
        out += struct.pack("<5f", wing["east"], wing["north"], wing["length"], wing["width"], wing["axis"])
    out += struct.pack("<3f", *entrance)
    out += struct.pack("<2f", *local(marks["lookout_tower"]["rd"]))
    out += struct.pack("<3f", *local(marks["start"]["rd"]), marks["start"]["heading"])
    out += struct.pack("<3f", *local(marks["lighthouse"]["rd"]), marks["lighthouse"]["height"])
    out += struct.pack("<I", len(kinds))
    out += triangles.astype("<f4").tobytes()
    out += kinds.tobytes()
    out += roadEras.tobytes()
    trees = json.load(open(TREES))["trees"] if os.path.exists(TREES) else []
    out += struct.pack("<I", len(trees))
    for tree in trees:
        out += struct.pack("<4f", *local((tree["x"], tree["y"])), tree["height"], tree["crown_radius"])

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "wb") as handle:
        handle.write(out)
    for level in levels:
        print("%-8s %3.0f m tiles, %d x %d nodes, %.1f to %.1f m NAP, %d tiles of water"
              % (level["name"], level["cell"], level["nx"], level["nz"], level["heights"].min(), level["heights"].max(),
                 int(((level["water"] != NO_WATER) & (level["water"] != SEA_MARK)).sum())))
    for level in levels:
        print("%-8s %d tiles of sea" % (level["name"], int((level["water"] == SEA_MARK).sum())))
    print("houses   %d, of which %d pitches; central building: %d wings cover %.0f%%"
          % (len(houses), sum(h["pitch"] for h in houses), len(wings), 100.0 * covered))
    print("roads    %d triangles: %s" % (len(kinds), ", ".join("%s %.0f m2" % item for item in sorted(areas.items()))))
    print("wrote    %s, %.1f MB" % (OUT, len(out) / 1e6))
    now = (roadEras & TODAY) != 0
    print("preview  %s" % previewRoads(levels[0], triangles[now], kinds[now], [h for h in houses if h["eras"] & TODAY]))
    then = (roadEras & OLD_WITH_CHALETS) != 0
    print("preview  %s" % previewRoads(levels[0], triangles[then], kinds[then], [h for h in houses if h["eras"] & OLD_WITH_CHALETS],
                                       "roads_1971_1993.png", "The roads of 1971 to 1993: those of the old park, and of today outside it."))


if __name__ == "__main__":
    main()
