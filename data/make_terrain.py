#!/usr/bin/env python3
# Slufter3D - makes the ground of the world from the AHN ground models: gaps filled, water and sea told apart
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/make_terrain.py
#
# The AHN ground model (DTM) has no height where the laser did not reach the ground: under buildings, on water
# and at sea. This script decides for every such gap which of the three it is, and writes for each area of
# tools/fetch_ahn.py (park 0.5 m, tip 2 m, horizon 5 m):
#   build/terrain/<area>_ground.tif   band 1: ground everywhere [m NAP]; band 2: level of the water surface,
#                                     no data on land
#   build/terrain/<area>_class.tif    0 measured ground, 1 ground filled in under a building, 2 inland water, 3 sea
# and build/terrain/terrain.json: what was found, the ponds, and the ground under every house of data/houses.json.
# Previews go to build/preview/terrain/. Everything here is derived: it can be made again from raw/ at any time.
#
# How a gap is told apart:
#   sea       gaps of the horizon model that reach its edge and are larger than SEA_MIN; finer models follow it
#   building  the surface model (DSM) has a height there: a roof. Without a surface model (horizon): small gaps
#   water     the rest: no ground and no surface
# Heights are the measured ones; nothing is scaled.
#
# Needs: numpy, scipy, rasterio, matplotlib (for the previews)
import json
import math
import os

import numpy as np
import rasterio
from scipy import ndimage

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RAW = os.path.join(ROOT, "raw", "ahn")
OUT = os.path.join(ROOT, "build", "terrain")
PREVIEW = os.path.join(ROOT, "build", "preview", "terrain")
HOUSES = os.path.join(ROOT, "data", "houses.json")

# Finest first; dsm None->there is no surface model of the area
AREAS = [
    {"name": "park", "dtm": "park_dtm_05m.tif", "dsm": "park_dsm_05m.tif"},
    {"name": "tip", "dtm": "tip_dtm_2m.tif", "dsm": "tip_dsm_2m.tif"},
    {"name": "horizon", "dtm": "horizon_dtm_5m.tif", "dsm": None},
]

MEASURED, BUILDING, WATER, SEA = 0, 1, 2, 3
SEA_MIN = 10000.0        # a gap at the edge of the horizon model is sea when larger than this [m2]
SEA_SHARE = 0.5          # a gap of a finer model is sea when this part of it lies in the sea of the horizon model
ROOF_SHARE = 0.5         # a gap is a building when the surface model has a height on this part of it
WATER_MIN = 25.0         # smaller gaps without a surface are noise, filled like buildings [m2]
BLIND_WATER_MIN = 5000.0 # without a surface model: larger gaps are water, smaller ones buildings [m2]
SEA_LEVEL = 0.0          # [m NAP]; the tide is left out
SEA_BED = -1.5           # ground under the sea, where nothing was measured [m NAP]
WATER_DEPTH = 0.4        # ground under inland water, below its surface [m]
BANK = 2                 # the level of a pond is read from the ground this many cells around it
BANK_PERCENTILE = 10.0   # low on the bank: at the waterline
SMOOTHING = 150          # relaxation steps of the ground filled in under buildings
FLOOR_LIFT = 0.15        # a floor lies this far above the ground it is set on [m]
FLOOR_PERCENTILE = 75.0  # of the ground along the outline: houses stand on the high side of their plot


def read(name):
    with rasterio.open(os.path.join(RAW, name)) as source:
        band = source.read(1, masked=True)
        return band.filled(np.nan).astype(np.float32), np.ma.getmaskarray(band), source.transform, source.crs


def seaOfHorizon():
    """Sea in the horizon model: its gaps that reach the edge and are large"""
    area = AREAS[-1]
    _, gap, transform, _ = read(area["dtm"])
    labels, count = ndimage.label(gap)
    cell = transform.a * -transform.e
    sizes = ndimage.sum(gap, labels, np.arange(1, count + 1)) * cell
    edge = np.unique(np.concatenate([labels[0], labels[-1], labels[:, 0], labels[:, -1]]))
    sea = np.isin(labels, [v for v in edge if v > 0 and sizes[v - 1] >= SEA_MIN])
    return sea, transform


def sampled(mask, transform, target, shape):
    """A mask on another grid: the value at the centre of each cell of the target grid"""
    rows, cols = np.indices(shape)
    x = target.c + (cols + 0.5) * target.a
    y = target.f + (rows + 0.5) * target.e
    r = np.clip(((y - transform.f) / transform.e).astype(int), 0, mask.shape[0] - 1)
    c = np.clip(((x - transform.c) / transform.a).astype(int), 0, mask.shape[1] - 1)
    return mask[r, c]


def relaxed(ground, holes):
    """Ground with the holes filled in: the nearest measured height, then smoothed within the holes"""
    nearest = ndimage.distance_transform_edt(holes, return_distances=False, return_indices=True)
    filled = ground[nearest[0], nearest[1]].astype(np.float32)
    if not holes.any():
        return filled
    # Only the surroundings of the holes are worked on
    rows = np.where(holes.any(axis=1))[0]
    cols = np.where(holes.any(axis=0))[0]
    r0, r1 = max(rows[0] - 1, 0), min(rows[-1] + 2, holes.shape[0])
    c0, c1 = max(cols[0] - 1, 0), min(cols[-1] + 2, holes.shape[1])
    part = filled[r0:r1, c0:c1]
    inside = holes[r0:r1, c0:c1]
    for _ in range(SMOOTHING):
        padded = np.pad(part, 1, mode="edge")
        mean = 0.25 * (padded[:-2, 1:-1] + padded[2:, 1:-1] + padded[1:-1, :-2] + padded[1:-1, 2:])
        part[inside] = mean[inside]
    return filled


def makeArea(area, sea, seaTransform):
    ground, gap, transform, crs = read(area["dtm"])
    cell = transform.a * -transform.e
    roof = None
    if area["dsm"]:
        _, noSurface, surfaceTransform, _ = read(area["dsm"])
        if surfaceTransform != transform or noSurface.shape != gap.shape:
            raise RuntimeError("The ground and surface models of %s are not on the same grid" % area["name"])
        roof = ~noSurface
    seaHere = sampled(sea, seaTransform, transform, gap.shape)

    labels, count = ndimage.label(gap)
    index = np.arange(1, count + 1)
    sizes = ndimage.sum(gap, labels, index) * cell
    seaShare = ndimage.mean(seaHere, labels, index) if count else np.array([])
    roofShare = ndimage.mean(roof, labels, index) if count and roof is not None else np.zeros(count)
    kind = np.full(count + 1, MEASURED, np.uint8)
    for v in range(1, count + 1):
        if seaShare[v - 1] >= SEA_SHARE:
            kind[v] = SEA
        elif roof is not None:
            kind[v] = BUILDING if roofShare[v - 1] >= ROOF_SHARE or sizes[v - 1] < WATER_MIN else WATER
        else:
            kind[v] = WATER if sizes[v - 1] >= BLIND_WATER_MIN else BUILDING
    classes = kind[labels]

    filled = relaxed(ground, classes == BUILDING)   # water and sea take the nearest height for now
    level = np.full(ground.shape, np.nan, np.float32)
    ponds = []
    structure = ndimage.generate_binary_structure(2, 2)
    slices = ndimage.find_objects(labels)
    for v in range(1, count + 1):
        if kind[v] != WATER:
            continue
        window = tuple(slice(max(s.start - BANK, 0), s.stop + BANK) for s in slices[v - 1])
        pond = labels[window] == v
        bank = ndimage.binary_dilation(pond, structure, iterations=BANK) & (classes[window] == MEASURED)
        if not bank.any():
            bank = ndimage.binary_dilation(pond, structure, iterations=BANK) & ~pond
        surface = float(np.percentile(filled[window][bank], BANK_PERCENTILE))
        level[window][pond] = surface
        filled[window][pond] = surface - WATER_DEPTH
        rows, cols = np.nonzero(pond)
        ponds.append({"x": round(float(transform.c + (window[1].start + cols.mean() + 0.5) * transform.a), 1),
                      "y": round(float(transform.f + (window[0].start + rows.mean() + 0.5) * transform.e), 1),
                      "area_m2": round(float(sizes[v - 1]), 0), "level_nap": round(surface, 2)})
    atSea = classes == SEA
    level[atSea] = SEA_LEVEL
    filled[atSea] = SEA_BED

    os.makedirs(OUT, exist_ok=True)
    profile = {"driver": "GTiff", "width": ground.shape[1], "height": ground.shape[0], "crs": crs, "transform": transform,
               "compress": "deflate", "tiled": True}
    with rasterio.open(os.path.join(OUT, area["name"] + "_ground.tif"), "w", count=2, dtype="float32",
                       nodata=float("nan"), predictor=3, **profile) as out:
        out.write(filled, 1)
        out.write(level, 2)
        out.set_band_description(1, "ground m NAP, gaps of the AHN filled in (tools/make_terrain.py)")
        out.set_band_description(2, "level of the water surface m NAP; no data on land")
    with rasterio.open(os.path.join(OUT, area["name"] + "_class.tif"), "w", count=1, dtype="uint8", **profile) as out:
        out.write(classes, 1)
        out.set_band_description(1, "0 measured, 1 filled in under a building, 2 inland water, 3 sea")

    total = float(classes.size)
    land = filled[classes != SEA]
    ponds.sort(key=lambda p: -p["area_m2"])
    summary = {
        "cell_m": transform.a,
        "bounds_rd": [transform.c, transform.f + ground.shape[0] * transform.e,
                      transform.c + ground.shape[1] * transform.a, transform.f],
        "share": {"measured": round(float((classes == MEASURED).sum() / total), 4),
                  "building": round(float((classes == BUILDING).sum() / total), 4),
                  "water": round(float((classes == WATER).sum() / total), 4),
                  "sea": round(float((classes == SEA).sum() / total), 4)},
        "land_min_nap": round(float(land.min()), 2), "land_max_nap": round(float(land.max()), 2),
        "ponds": ponds,
    }
    print("%-8s %4.1f m cells: measured %.1f%%, under buildings %.1f%%, water %.1f%% (%d), sea %.1f%%; land %.1f to %.1f m NAP"
          % (area["name"], transform.a, 100 * summary["share"]["measured"], 100 * summary["share"]["building"],
             100 * summary["share"]["water"], len(ponds), 100 * summary["share"]["sea"], summary["land_min_nap"],
             summary["land_max_nap"]))
    return summary, filled, level, classes, transform


def houseGround(houses, ground, classes, transform):
    """The ground under every house: at its centre and along its outline; and the floor that follows from it"""
    def at(x, y):
        r = np.clip(((np.asarray(y) - transform.f) / transform.e).astype(int), 0, ground.shape[0] - 1)
        c = np.clip(((np.asarray(x) - transform.c) / transform.a).astype(int), 0, ground.shape[1] - 1)
        return ground[r, c], classes[r, c]

    floors = []
    for house in houses:
        a = math.radians(house["axis_azimuth"])
        along = np.array([math.sin(a), math.cos(a)])
        across = np.array([math.cos(a), -math.sin(a)])
        t = np.linspace(-0.5, 0.5, 17)
        edge = [(u * house["length"], sv * 0.5 * house["width"]) for u in t for sv in (-1, 1)]
        edge += [(su * 0.5 * house["length"], v * house["width"]) for v in t for su in (-1, 1)]
        px = house["x"] + np.array([u * along[0] + v * across[0] for u, v in edge])
        py = house["y"] + np.array([u * along[1] + v * across[1] for u, v in edge])
        heights, kinds = at(px, py)
        centre, _ = at(house["x"], house["y"])
        floors.append({"nr": house["nr"], "type": house["type"], "x": house["x"], "y": house["y"],
                       "ground_centre": round(float(centre), 2), "ground_low": round(float(heights.min()), 2),
                       "ground_high": round(float(heights.max()), 2),
                       "floor": round(float(np.percentile(heights, FLOOR_PERCENTILE)) + FLOOR_LIFT, 2),
                       "in_water": bool((kinds >= WATER).any())})
    # Both halves of a block stand on one floor: the higher one
    byNumber = {f["nr"]: f for f in floors if f["nr"]}
    for house, floor in zip(houses, floors):
        other = byNumber.get(house.get("semi_with"))
        if other:
            floor["floor"] = other["floor"] = max(floor["floor"], other["floor"])
    return floors


def preview(name, ground, level, classes, transform, houses):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.colors import LightSource, Normalize
    step = max(1, int(round(max(ground.shape) / 2400.0)))
    g, c = ground[::step, ::step], classes[::step, ::step]
    cell = transform.a * step
    land = g[c != SEA]
    low, high = np.percentile(land, [0.5, 99.8])
    shade = LightSource(azdeg=315, altdeg=40).shade(g, cmap=matplotlib.colormaps["gist_earth"], norm=Normalize(low - 6.0, high),
                                                     blend_mode="soft", vert_exag=2.0, dx=cell, dy=cell)[..., :3]
    shade[c == SEA] = (0.55, 0.70, 0.85)
    shade[c == WATER] = (0.25, 0.50, 0.85)
    shade[c == BUILDING] = 0.6 * shade[c == BUILDING] + 0.4 * np.array([0.75, 0.30, 0.55])
    west, north = transform.c, transform.f
    east, south = west + ground.shape[1] * transform.a, north + ground.shape[0] * transform.e
    figure, axis = plt.subplots(figsize=(12.0, 12.0 * (north - south) / (east - west)), dpi=150)
    axis.imshow(shade, extent=(west, east, south, north), origin="upper", interpolation="nearest")
    if name == "park":
        for house in houses:
            axis.text(house["x"], house["y"], str(house["nr"]) if house["nr"] else "", fontsize=2.5, ha="center",
                      va="center", color="white")
    axis.set_title("%s: ground with the gaps filled in, %.1f to %.1f m NAP. Blue: water, pale blue: sea, "
                   "purple tint: filled in under a building" % (name, land.min(), land.max()), fontsize=9)
    axis.set_xlabel("RD x (east) [m]")
    axis.set_ylabel("RD y (north) [m]")
    os.makedirs(PREVIEW, exist_ok=True)
    out = os.path.join(PREVIEW, name + ".png")
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


def main():
    sea, seaTransform = seaOfHorizon()
    houses = json.load(open(HOUSES))["houses"] if os.path.exists(HOUSES) else []
    summary = {"note": "Made by tools/make_terrain.py from raw/ahn/; derived, never edited. Heights in m NAP, as measured.",
               "classes": {"0": "measured ground", "1": "filled in under a building", "2": "inland water", "3": "sea"},
               "areas": {}}
    for area in AREAS:
        result, ground, level, classes, transform = makeArea(area, sea, seaTransform)
        summary["areas"][area["name"]] = result
        if area["name"] == "park" and houses:
            floors = houseGround(houses, ground, classes, transform)
            summary["houses"] = floors
            wet = [f for f in floors if f["in_water"]]
            steep = sorted(floors, key=lambda f: f["ground_low"] - f["ground_high"])[:5]
            print("         houses: floors %.1f to %.1f m NAP; %d with water under the outline; steepest plots: %s"
                  % (min(f["floor"] for f in floors), max(f["floor"] for f in floors), len(wet),
                     ", ".join("%s %.1f m" % (f["nr"] or f["type"], f["ground_high"] - f["ground_low"]) for f in steep)))
        print("         preview %s" % preview(area["name"], ground, level, classes, transform, houses))
    with open(os.path.join(OUT, "terrain.json"), "w") as handle:
        json.dump(summary, handle, indent=1)
        handle.write("\n")


if __name__ == "__main__":
    main()
