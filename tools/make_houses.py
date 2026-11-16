#!/usr/bin/env python3
# Slufter3D - places the houses of the park on the AHN height models: data/houses.json
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/make_houses.py
#
# What comes from where:
#   position, size and axis of a building   the AHN rasters (raw/ahn/): a building is where the surface model has
#                                           a height and the ground model has none
#   number, type, which side is the front   the house table of the first application (raw/slufter_v1/houses.json),
#                                           which took numbers and types from the park's Plattegrond
#   camping pitches                         that table only; tents are not in the AHN
# The old table is about 16% too large in position and shifted. Its houses are first laid over the AHN buildings
# with one scale, rotation and shift for the whole park; then every house is fitted to its own building.
# Houses whose fit is doubtful are listed in build/review/houses_review.md and drawn red in houses_review.png.
# Decisions taken after a review go into data/house_corrections.json (optional), never into the raw files.
#
# Needs: numpy, scipy, rasterio, shapely, opencv-python
import copy
import json
import math
import os

import cv2
import numpy as np
import rasterio
import rasterio.features
from scipy import ndimage, optimize
from scipy.spatial import cKDTree
from shapely.geometry import MultiPoint, Point, Polygon, shape

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
DTM = os.path.join(ROOT, "raw", "ahn", "park_dtm_05m.tif")
DSM = os.path.join(ROOT, "raw", "ahn", "park_dsm_05m.tif")
OLD = os.path.join(ROOT, "raw", "slufter_v1", "houses.json")
CORRECTIONS = os.path.join(ROOT, "data", "house_corrections.json")
OUT = os.path.join(ROOT, "data", "houses.json")
REVIEW_DIR = os.path.join(ROOT, "build", "review")

MIN_BLOCK = 20.0        # smallest AHN building used for the park-wide fit [m2]
MAX_HOUSE = 400.0       # largest; the central building is left out of that fit [m2]
SAMPLE = 0.5            # spacing of the points a rectangle is scored on [m]
SEARCH = 6.0            # a house is looked for this far from where the park-wide fit puts it [m]
SEARCH_STEP = 1.0
TURN = 20.0             # and turned at most this far [deg]
SIZE_FREEDOM = 0.45     # length and width may differ this much from the old table
SQUARE = 1.15           # a house is turned a quarter only if it is longer than wide by this factor
WHOLE_BLOCK = 15.0      # a comfort house longer than this is a block of two [m]
FITTED_PITCHES = ("6ST",)   # pitches with a structure that the AHN shows: safari tents
SEMI_REACH = 14.0       # the two halves of a block lie within this distance in the old table [m]
REVIEW_FILL = 0.80      # part of a fitted rectangle that must be building
REVIEW_COVER = 0.70     # part of its AHN building that the houses on it must cover
REVIEW_MOVE = 4.0       # [m]
REVIEW_AREA = 0.6       # an outline this much smaller or larger than in the old table may be another building
PIXEL = 0.25            # of the review picture [m]


# ---------------------------------------------------------------- AHN buildings

def loadBuildings():
    with rasterio.open(DTM) as dtmSource, rasterio.open(DSM) as dsmSource:
        if dtmSource.transform != dsmSource.transform or dtmSource.shape != dsmSource.shape:
            raise RuntimeError("The ground and surface models of the park are not on the same grid")
        ground = np.ma.getmaskarray(dtmSource.read(1, masked=True))
        surface = np.ma.getmaskarray(dsmSource.read(1, masked=True))
        transform = dtmSource.transform
        bounds = dtmSource.bounds
    # Specks are shrubs and noise; opening removes them and keeps the roofs
    mask = ndimage.binary_opening(ground & ~surface, iterations=2)
    return mask, transform, bounds


class Field:
    """+1 on a building, -1 elsewhere, read at any point in RD New"""

    def __init__(self, mask, transform, bounds):
        self.m_mask = mask
        self.m_signed = ndimage.gaussian_filter(np.where(mask, 1.0, -1.0), 0.7)
        self.m_transform = transform
        self.m_cell = transform.a
        self.m_west = bounds.left
        self.m_north = bounds.top
        self.m_labels, self.m_count = ndimage.label(mask)
        index = np.arange(1, self.m_count + 1)
        self.m_areas = ndimage.sum(mask, self.m_labels, index) * self.m_cell ** 2
        centres = np.array(ndimage.center_of_mass(mask, self.m_labels, index))
        self.m_centres = np.c_[self.m_west + centres[:, 1] * self.m_cell, self.m_north - centres[:, 0] * self.m_cell]

    def pixels(self, x, y):
        return (self.m_north - np.asarray(y)) / self.m_cell - 0.5, (np.asarray(x) - self.m_west) / self.m_cell - 0.5

    def signed(self, x, y):
        rows, cols = self.pixels(x, y)
        return ndimage.map_coordinates(self.m_signed, [rows, cols], order=1, mode="constant", cval=-1.0)

    def labelsAt(self, x, y):
        rows, cols = self.pixels(x, y)
        rows = np.clip(np.round(rows).astype(int), 0, self.m_mask.shape[0] - 1)
        cols = np.clip(np.round(cols).astype(int), 0, self.m_mask.shape[1] - 1)
        return self.m_labels[rows, cols]


# ---------------------------------------------------------------- rectangles

def axes(azimuth):
    """Unit vectors along the long axis and 90 degrees clockwise of it; azimuth clockwise from north [deg]"""
    a = math.radians(azimuth)
    return np.array([math.sin(a), math.cos(a)]), np.array([math.cos(a), -math.sin(a)])


def samplePoints(x, y, length, width, azimuth):
    along, across = axes(azimuth)
    nu = max(2, int(round(length / SAMPLE)))
    nv = max(2, int(round(width / SAMPLE)))
    u = (np.arange(nu) + 0.5) / nu * length - 0.5 * length
    v = (np.arange(nv) + 0.5) / nv * width - 0.5 * width
    uu, vv = np.meshgrid(u, v)
    px = x + uu * along[0] + vv * across[0]
    py = y + uu * along[1] + vv * across[1]
    return px.ravel(), py.ravel(), length * width / (nu * nv)


def corners(x, y, length, width, azimuth):
    along, across = axes(azimuth)
    centre = np.array([x, y])
    return [centre + su * 0.5 * length * along + sv * 0.5 * width * across
            for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1))]


def score(field, x, y, length, width, azimuth):
    """Area of building under the rectangle less the area of anything else under it [m2]"""
    px, py, area = samplePoints(x, y, length, width, azimuth)
    return float(field.signed(px, py).sum() * area)


def fitRectangle(field, start):
    """The rectangle near start = (x, y, length, width, azimuth) that best covers a building"""
    x0, y0, length0, width0, azimuth0 = start
    best = (-1.0e9, x0, y0)
    steps = np.arange(-SEARCH, SEARCH + 0.5 * SEARCH_STEP, SEARCH_STEP)
    for dx in steps:
        for dy in steps:
            # Nearer places win when two are equally good
            value = score(field, x0 + dx, y0 + dy, length0, width0, azimuth0) - 0.5 * math.hypot(dx, dy)
            if value > best[0]:
                best = (value, x0 + dx, y0 + dy)
    low = [best[1] - 2.0, best[2] - 2.0, (1.0 - SIZE_FREEDOM) * length0, (1.0 - SIZE_FREEDOM) * width0, azimuth0 - TURN]
    high = [best[1] + 2.0, best[2] + 2.0, (1.0 + SIZE_FREEDOM) * length0, (1.0 + SIZE_FREEDOM) * width0, azimuth0 + TURN]

    def cost(p):
        q = np.clip(p, low, high)
        return -score(field, *q) + 10.0 * float(np.abs(np.asarray(p) - q).sum())

    result = optimize.minimize(cost, [best[1], best[2], length0, width0, azimuth0], method="Nelder-Mead",
                               options={"xatol": 0.02, "fatol": 0.02, "maxiter": 1500,
                                        "initial_simplex": simplex([best[1], best[2], length0, width0, azimuth0])})
    return tuple(float(v) for v in np.clip(result.x, low, high))


def simplex(start):
    points = [np.array(start, dtype=float)]
    for k, step in enumerate((1.0, 1.0, 1.0, 1.0, 5.0)):
        p = np.array(start, dtype=float)
        p[k] += step
        points.append(p)
    return np.array(points)


def fill(field, rect):
    px, py, _ = samplePoints(*rect)
    return float((field.labelsAt(px, py) > 0).mean())


# ---------------------------------------------------------------- the park-wide fit

def parkFit(field, houses):
    """Scale, rotation and shift that lay the old table over the AHN buildings: p -> M p + t"""
    old = np.array([[h["x"], h["y"]] for h in houses if h["kind"] == "building"])
    keep = (field.m_areas > MIN_BLOCK) & (field.m_areas < MAX_HOUSE)
    blocks = field.m_centres[keep]
    tree = cKDTree(blocks)
    best = None
    for scale in np.arange(0.70, 1.11, 0.02):
        for shiftX in np.arange(-80.0, 81.0, 20.0):
            for shiftY in np.arange(-80.0, 81.0, 20.0):
                M = scale * np.eye(2)
                t = old.mean(0) + np.array([shiftX, shiftY]) - M @ old.mean(0)
                for step in range(40):
                    moved = old @ M.T + t
                    distance, nearest = tree.query(moved)
                    near = distance < max(6.0, 25.0 - step)
                    if near.sum() < 50:
                        break
                    p, q = old[near], blocks[nearest[near]]
                    pm, qm = p.mean(0), q.mean(0)
                    U, S, Vt = np.linalg.svd((p - pm).T @ (q - qm))
                    R = Vt.T @ U.T
                    if np.linalg.det(R) < 0.0:
                        Vt[1] *= -1.0
                        R = Vt.T @ U.T
                    M = S.sum() / ((p - pm) ** 2).sum() * R
                    t = qm - M @ pm
                distance, _ = tree.query(old @ M.T + t)
                hits = int((distance < 4.0).sum())
                if best is None or hits > best[0]:
                    best = (hits, M.copy(), t.copy(), float(np.median(distance)))
    hits, M, t, median = best
    return M, t, {"scale": round(float(math.sqrt(np.linalg.det(M))), 5),
                  "rotation_deg": round(math.degrees(math.atan2(M[1, 0], M[0, 0])), 3),
                  "matrix": [[round(float(v), 6) for v in row] for row in M],
                  "shift": [round(float(v), 2) for v in t],
                  "houses_within_4m_of_a_building": hits, "houses": len(old),
                  "median_distance_m": round(median, 2),
                  "note": "true RD = matrix * old + shift, for the frame of raw/slufter_v1/houses.json"}


# ---------------------------------------------------------------- houses

def nearestTurn(angle, target, period):
    """angle + k*period nearest to target [deg]"""
    return angle + period * round((target - angle) / period)


def pairSemis(houses):
    """Index of the other half for every half of a semi-detached block, by the old table"""
    partner = {}
    semis = [k for k, h in enumerate(houses) if h["semi"] and h["kind"] == "building"]
    for k in semis:
        if k in partner:
            continue
        h = houses[k]
        along, across = axes(h["axis_azimuth"])
        best = None
        for j in semis:
            if j == k or j in partner:
                continue
            d = np.array([houses[j]["x"] - h["x"], houses[j]["y"] - h["y"]])
            if abs(d @ across) < 1.5 and np.hypot(*d) < SEMI_REACH and (best is None or np.hypot(*d) < best[0]):
                best = (np.hypot(*d), j)
        if best:
            partner[k] = best[1]
            partner[best[1]] = k
    return partner


def blanked(field, polygons):
    """The field without the buildings under these rectangles, which other houses stand on"""
    other = copy.copy(field)
    taken = rasterio.features.rasterize([(p.buffer(0.5), 1) for p in polygons], out_shape=field.m_mask.shape,
                                        transform=field.m_transform).astype(bool)
    other.m_signed = np.where(taken, -1.0, field.m_signed)
    return other


def place(field, houses, group, moved, whole=None):
    """Fits one house, or the two halves of a block, to the AHN; returns {index: record}, {index: rectangle}.
    whole: the rectangle of the block when it is known already"""
    k = group[0]
    h = houses[k]
    p, axis, _ = moved(h)
    if whole is not None:
        start = whole
        rect = whole
        turned = False
    elif len(group) == 2:
        q, _, _ = moved(houses[group[1]])
        start = (0.5 * (p[0] + q[0]), 0.5 * (p[1] + q[1]), h["length"] + houses[group[1]]["length"],
                 max(h["width"], houses[group[1]]["width"]), axis)
        rect = fitRectangle(field, start)
        # A block may lie a quarter turn from where the old table has it
        other = fitRectangle(field, (start[0], start[1], start[2], start[3], axis + 90.0))
        turned = score(field, *other) > 1.10 * score(field, *rect) + 2.0
        if turned:
            rect = other
    else:
        start = (p[0], p[1], h["length"], h["width"], axis)
        rect = fitRectangle(field, start)
        # The old table may have the long axis the wrong way round: the same rectangle a quarter turn on
        other = fitRectangle(field, (p[0], p[1], h["length"], h["width"], axis + 90.0))
        turned = score(field, *other) > 1.10 * score(field, *rect) + 2.0 and h["length"] > SQUARE * h["width"] \
            and not h["front_known"]
        if turned:
            rect = other
    filled = fill(field, rect)
    found = filled >= 0.5
    if not found:
        rect = start
        turned = False
    x, y, length, width, azimuth = rect
    if found and len(group) == 1 and width > SQUARE * length and not h["front_known"]:
        # The long axis is the other one: the same rectangle, named the other way round
        length, width, azimuth = width, length, azimuth + 90.0
        rect = (x, y, length, width, azimuth)
        turned = True
        start = (start[0], start[1], start[3], start[2], start[4] + 90.0)
    along, _ = axes(azimuth)
    # The AHN building(s) under the rectangle, and how much of them it covers
    px, py, cellArea = samplePoints(*rect)
    labels = field.labelsAt(px, py)
    under = [int(v) for v in np.unique(labels[labels > 0])
             if (labels == v).sum() * cellArea > 0.25 * min(length * width, field.m_areas[v - 1])]
    blockArea = float(sum(field.m_areas[v - 1] for v in under))
    cover = float(np.isin(labels, under).sum() * cellArea / blockArea) if blockArea > 0.0 else 0.0
    atLimit = []
    for name, value, was in (("length", length, start[2]), ("width", width, start[3])):
        # Only worth a look when the rectangle and the building do not agree
        if found and abs(value / was - 1.0) > SIZE_FREEDOM - 0.01 and min(filled, cover) < 0.85:
            atLimit.append("%s at the limit of the search (%.1f m, %.1f m in the v1 table)" % (name, value, was))
    records = {}
    rects = {}
    first = 1.0 if (p - np.array([x, y])) @ along > 0.0 else -1.0
    for index, j in enumerate(group):
        hj = houses[j]
        pj, axisJ, frontJ = moved(hj)
        if len(group) == 2:
            side = first if index == 0 else -first
            centre = np.array([x, y]) + side * 0.25 * length * along
            size = (0.5 * length, width)
        else:
            centre = np.array([x, y])
            size = (length, width)
        axisNew = nearestTurn(azimuth, axisJ, 180.0)
        frontNew = min((axisNew + 90.0, axisNew - 90.0), key=lambda a: abs((a - frontJ + 180.0) % 360.0 - 180.0))
        reasons = []
        move = float(np.hypot(*(centre - pj)))
        if not found:
            if hj["kind"] == "building":
                reasons.append("no AHN building found within %.0f m; position is that of the v1 table" % SEARCH)
        else:
            if filled < REVIEW_FILL:
                reasons.append("only %.0f%% of the rectangle is building" % (100.0 * filled))
            if cover < REVIEW_COVER:
                reasons.append("covers only %.0f%% of its AHN building (%.0f m2): larger or joined to another"
                               % (100.0 * cover, blockArea))
            if move > REVIEW_MOVE and len(group) == 1:
                reasons.append("moved %.1f m from where the park-wide fit put it" % move)
            ratio = size[0] * size[1] / (hj["length"] * hj["width"])
            if len(group) == 1 and not (REVIEW_AREA < ratio < 1.0 / REVIEW_AREA):
                reasons.append("outline %.0f m2, %.0f m2 in the v1 table: perhaps another building"
                               % (size[0] * size[1], hj["length"] * hj["width"]))
            if turned:
                reasons.append("long axis a quarter turn from the v1 table; check the front")
            reasons += atLimit
        record = {"nr": hj["nr"], "type": hj["type"], "name": hj["name"], "kind": hj["kind"],
                  "x": round(float(centre[0]), 2), "y": round(float(centre[1]), 2),
                  "length": round(size[0], 1), "width": round(size[1], 1),
                  "axis_azimuth": round(axisNew % 360.0, 1), "front_azimuth": round(frontNew % 360.0, 1),
                  "source": {"position": "AHN building" if found else "v1 table, moved by the park-wide fit",
                             "size": "AHN building (roof outline)" if found else "v1 table",
                             "front": "v1 table" if hj["front_known"] else "v1 rule: the side nearest south"},
                  "fit": {"building_part": round(filled, 2), "covers": round(cover, 2), "moved_m": round(move, 1)},
                  "review": reasons}
        if len(group) == 2:
            record["semi_with"] = houses[group[1 - index]]["nr"]
            if not hj["semi"]:
                record["review"].append("fitted as half of one AHN building with %d; not a block in the v1 table"
                                        % houses[group[1 - index]]["nr"])
        records[j] = record
        rects[j] = Polygon(corners(centre[0], centre[1], size[0], size[1], azimuth))
    return records, rects


def main():
    old = json.load(open(OLD))
    houses = old["houses"]
    corrections = json.load(open(CORRECTIONS)) if os.path.exists(CORRECTIONS) else {}
    accepted = set(corrections.get("accepted", []))
    mask, transform, bounds = loadBuildings()
    field = Field(mask, transform, bounds)
    M, t, fit = parkFit(field, houses)
    turn = fit["rotation_deg"]
    print("park-wide fit: scale %.4f, rotation %.2f deg, %d of %d houses within 4 m of a building"
          % (fit["scale"], turn, fit["houses_within_4m_of_a_building"], fit["houses"]))

    def moved(h):
        p = M @ np.array([h["x"], h["y"]]) + t
        return p, h["axis_azimuth"] - turn, h["front_azimuth"] - turn

    # The central building: the AHN building under its old outline; no house is fitted to it
    central = None
    if "central_building" in old:
        ring = np.array(old["central_building"]["outline"]) @ M.T + t
        inside = rasterio.features.rasterize([(Polygon(ring), 1)], out_shape=mask.shape, transform=transform).astype(bool)
        labels = field.m_labels[inside & mask]
        if labels.size:
            v = int(np.bincount(labels).argmax())
            shapes = [shape(g) for g, value in rasterio.features.shapes((field.m_labels == v).astype(np.uint8),
                                                                        transform=transform) if value == 1]
            outline = max(shapes, key=lambda s: s.area).simplify(0.6)
            central = {"type": "central_facilities", "kind": "central",
                       "x": round(outline.centroid.x, 2), "y": round(outline.centroid.y, 2),
                       "area_m2": round(outline.area, 0),
                       "outline": [[round(x, 2), round(y, 2)] for x, y in outline.exterior.coords],
                       "source": {"position": "AHN building", "outline": "AHN building (roof outline)"}}
            field = blanked(field, [outline])
            centralLabel = v

    partner = pairSemis(houses)
    records = [None] * len(houses)
    rects = {}
    groups = {}
    for k, h in enumerate(houses):
        if records[k] is not None:
            continue
        if h["type"] not in FITTED_PITCHES and h["kind"] != "building":
            p, axis, front = moved(h)
            records[k] = {"nr": h["nr"], "type": h["type"], "name": h["name"], "kind": h["kind"],
                          "x": round(float(p[0]), 2), "y": round(float(p[1]), 2), "length": h["length"], "width": h["width"],
                          "axis_azimuth": round(axis % 360.0, 1), "front_azimuth": round(front % 360.0, 1),
                          "source": {"position": "v1 table, moved by the park-wide fit", "size": "v1 table",
                                     "front": "v1 table"}, "review": []}
            continue
        group = [k] + ([partner[k]] if k in partner else [])
        placed, shapes = place(field, houses, group, moved)
        for j in group:
            records[j] = placed[j]
            groups[j] = group
        rects.update(shapes)

    # Two houses on one building: the one that came from further away looks again, without the other's building
    for attempt in range(4):
        keys = sorted(rects)
        tree = cKDTree(np.array([[records[k]["x"], records[k]["y"]] for k in keys]))
        clashes = []
        for a, b in tree.query_pairs(25.0):
            ka, kb = keys[a], keys[b]
            if kb in groups[ka]:
                continue
            if rects[ka].intersection(rects[kb]).area > 0.2 * min(rects[ka].area, rects[kb].area):
                clashes.append((ka, kb))
        if not clashes:
            break
        redone = set()
        for ka, kb in clashes:
            loser, winner = (ka, kb) if records[ka]["fit"]["moved_m"] >= records[kb]["fit"]["moved_m"] else (kb, ka)
            if loser in redone or winner in redone:
                continue
            if len(groups[ka]) == 1 and len(groups[kb]) == 1 and records[winner]["length"] > WHOLE_BLOCK:
                # One long building under two houses: the two halves of a block, which the old table gave to one
                pair = [winner, loser]
                r = records[winner]
                placed, shapes = place(field, houses, pair, moved,
                                       whole=(r["x"], r["y"], r["length"], r["width"], r["axis_azimuth"]))
                for j in pair:
                    records[j] = placed[j]
                    groups[j] = pair
                    redone.add(j)
                rects.update(shapes)
                continue
            others = [rects[k] for k in keys if k not in groups[loser]
                      and rects[k].distance(rects[loser]) < 2.0 * SEARCH]
            placed, shapes = place(blanked(field, others), houses, groups[loser], moved)
            for j in groups[loser]:
                records[j] = placed[j]
                redone.add(j)
            rects.update(shapes)
    keys = sorted(rects)
    tree = cKDTree(np.array([[records[k]["x"], records[k]["y"]] for k in keys]))
    for a, b in tree.query_pairs(25.0):
        ka, kb = keys[a], keys[b]
        if kb not in groups[ka] and rects[ka].intersection(rects[kb]).area > 0.2 * min(rects[ka].area, rects[kb].area):
            records[ka]["review"].append("overlaps %s" % label(records[kb]))
            records[kb]["review"].append("overlaps %s" % label(records[ka]))

    # AHN buildings among the houses that no house stands on
    points = np.array([[r["x"], r["y"]] for r in records if r["kind"] == "building"])
    named = [r for r in records if r["kind"] == "building"]
    houseTree = cKDTree(points)
    hull = MultiPoint(points).convex_hull.buffer(15.0)
    covered = np.zeros(field.m_count + 1, bool)
    if central:
        covered[centralLabel] = True
    for k in keys:
        r = records[k]
        px, py, _ = samplePoints(r["x"], r["y"], r["length"], r["width"], r["axis_azimuth"])
        covered[np.unique(field.labelsAt(px, py))] = True
    unassigned = []
    for v in range(1, field.m_count + 1):
        centre = field.m_centres[v - 1]
        if not covered[v] and field.m_areas[v - 1] >= MIN_BLOCK and hull.contains(Point(*centre)):
            unassigned.append({"x": round(float(centre[0]), 1), "y": round(float(centre[1]), 1),
                               "area_m2": round(float(field.m_areas[v - 1]), 0),
                               "nearest_house": label(named[houseTree.query(centre)[1]])})

    for r in records:
        if label(r) in accepted:
            r["accepted"] = r["review"]
            r["review"] = []
    records.sort(key=lambda r: (r["kind"] != "building", r["nr"] == 0, r["nr"], r["type"], r["x"]))
    out = {
        "note": "The houses and pitches of Landal Sluftervallei in RD New (EPSG:28992), metres. Generated by"
                " tools/make_houses.py; do not edit. x, y: centre of the outline. length, width: the outline, which"
                " for buildings is that of the roof. axis_azimuth: of the long axis; front_azimuth: the way the front"
                " faces; both in degrees clockwise from north. source says where each value comes from; review lists"
                " what should be checked by eye (empty: nothing).",
        "park_wide_fit": fit,
        "houses": records,
        "central_building": central,
        "ahn_buildings_without_a_house": unassigned,
    }
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as handle:
        json.dump(out, handle, indent=1)
        handle.write("\n")
    review = [r for r in records if r["review"]]
    buildings = [r for r in records if r["kind"] == "building"]
    print("houses: %d buildings (%d on an AHN building), %d pitches; %d to review; %d AHN buildings without a house"
          % (len(buildings), sum(r["source"]["position"] == "AHN building" for r in buildings),
             len(records) - len(buildings), len(review), len(unassigned)))
    writeReview(records, unassigned, central, fit, field)


def label(r):
    if r["nr"]:
        return "%d (%s)" % (r["nr"], r["type"])
    name = r["name"] if r["kind"] == "building" and r["name"] else r["type"]
    return "%s at %.0f, %.0f" % (name, r["x"], r["y"])


# ---------------------------------------------------------------- review

def writeReview(records, unassigned, central, fit, field):
    os.makedirs(REVIEW_DIR, exist_ok=True)
    review = [r for r in records if r["review"]]
    lines = ["# Houses to review", "",
             "Generated by `tools/make_houses.py`. Park-wide fit of the old table: scale %.4f, rotation %.2f deg." % (
                 fit["scale"], fit["rotation_deg"]),
             "In `houses_review.png`: green is a house on its AHN building, red one listed here, blue a pitch,",
             "orange an AHN building without a house. The tick shows the front.", "",
             "To accept a house as it is, add its label to `accepted` in `data/house_corrections.json`.", "",
             "| House | RD x | RD y | Why |", "|---|---|---|---|"]
    for r in review:
        lines.append("| %s | %.1f | %.1f | %s |" % (label(r), r["x"], r["y"], "; ".join(r["review"])))
    lines += ["", "## AHN buildings without a house", "", "| RD x | RD y | Area [m2] | Nearest house |", "|---|---|---|---|"]
    for u in unassigned:
        lines.append("| %.1f | %.1f | %.0f | %s |" % (u["x"], u["y"], u["area_m2"], u["nearest_house"]))
    with open(os.path.join(REVIEW_DIR, "houses_review.md"), "w") as handle:
        handle.write("\n".join(lines) + "\n")

    xs = [r["x"] for r in records]
    ys = [r["y"] for r in records]
    west, east = min(xs) - 40.0, max(xs) + 40.0
    south, north = min(ys) - 40.0, max(ys) + 40.0
    width = int((east - west) / PIXEL)
    height = int((north - south) / PIXEL)
    gx = west + (np.arange(width) + 0.5) * PIXEL
    gy = north - (np.arange(height) + 0.5) * PIXEL
    rows, cols = field.pixels(*np.meshgrid(gx, gy))
    rows = np.clip(np.round(rows).astype(int), 0, field.m_mask.shape[0] - 1)
    cols = np.clip(np.round(cols).astype(int), 0, field.m_mask.shape[1] - 1)
    image = np.full((height, width, 3), 248, np.uint8)
    image[field.m_mask[rows, cols]] = (215, 170, 200)

    def pixel(p):
        return [int(round((p[0] - west) / PIXEL)), int(round((north - p[1]) / PIXEL))]

    for u in unassigned:
        cv2.circle(image, tuple(pixel((u["x"], u["y"]))), 24, (0, 140, 255), 3)
    if central:
        cv2.polylines(image, [np.array([pixel(p) for p in central["outline"]], np.int32)], True, (120, 60, 0), 3)
    for r in records:
        colour = (200, 90, 0) if r["kind"] != "building" else ((0, 0, 220) if r["review"] else (0, 130, 0))
        ring = corners(r["x"], r["y"], r["length"], r["width"], r["axis_azimuth"])
        cv2.polylines(image, [np.array([pixel(p) for p in ring], np.int32)], True, colour, 2)
        a = math.radians(r["front_azimuth"])
        reach = 0.5 * r["width"]
        tip = (r["x"] + (reach + 2.0) * math.sin(a), r["y"] + (reach + 2.0) * math.cos(a))
        foot = (r["x"] + reach * math.sin(a), r["y"] + reach * math.cos(a))
        cv2.line(image, tuple(pixel(foot)), tuple(pixel(tip)), colour, 3)
        text = str(r["nr"]) if r["nr"] else (r["name"] if r["kind"] == "building" and r["name"] else r["type"])
        size = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)[0]
        at = pixel((r["x"], r["y"]))
        cv2.putText(image, text, (at[0] - size[0] // 2, at[1] + size[1] // 2), cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                    (0, 0, 0), 1, cv2.LINE_AA)
    cv2.imwrite(os.path.join(REVIEW_DIR, "houses_review.png"), image)
    print("review: %s" % os.path.join(REVIEW_DIR, "houses_review.md"))


if __name__ == "__main__":
    main()
