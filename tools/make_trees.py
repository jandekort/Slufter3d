#!/usr/bin/env python3
# Slufter3D - finds the trees of the park in the AHN: what stands above the ground and is not a building
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/make_trees.py [--threshold 1.0] [--species scots_pine]
#        (run tools/make_terrain.py first: it makes the ground under the buildings)
#
# The height of what stands on the ground is the surface model (DSM) minus the ground. Buildings, tents and
# water are left out; what remains above the threshold is taken to be a tree or a shrub. Every top of that
# canopy becomes one tree; the canopy is shared out over the tops, which gives each tree the width of its crown.
# Writes data/trees.json and a picture to check it by, build/preview/trees/trees.png.
#
# What this cannot tell apart: a parked car, a fence or a caravan is as high as a shrub. Raise the threshold
# to lose them (and the shrubs). Trees closer together than TOP_DISTANCE come out as one.
#
# Needs: numpy, scipy, rasterio, matplotlib (for the preview)
import argparse
import json
import math
import os

import numpy as np
import rasterio
from rasterio import features as rasterfeatures
from scipy import ndimage
from scipy.spatial import cKDTree

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SURFACE = os.path.join(ROOT, "raw", "ahn", "park_dsm_05m.tif")
GROUND = os.path.join(ROOT, "build", "terrain", "park_ground.tif")
CLASSES = os.path.join(ROOT, "build", "terrain", "park_class.tif")
BGT = os.path.join(ROOT, "raw", "bgt")
HOUSES = os.path.join(ROOT, "data", "houses.json")
OUT = os.path.join(ROOT, "data", "trees.json")
PREVIEW = os.path.join(ROOT, "build", "preview", "trees")

BUILDING, WATER, SEA = 1, 2, 3   # classes of tools/make_terrain.py
ROOF_ROUGHNESS = 0.15    # a roof is smooth: the surface differs less than this from the mean around it [m]
THRESHOLD = 1.0          # lower than this above the ground is not a tree [m]
BUILDING_MARGIN = 1.0    # around buildings nothing is a tree: eaves, and the edge of the roof in the raster [m]
HOUSE_MARGIN = 0.75      # the same around the houses and tents of data/houses.json [m]
SMOOTH = 0.75            # the canopy is smoothed over this distance before its tops are sought [m]
TOP_DISTANCE = 2.0       # two tops closer than this are one tree [m]
CROWN_MIN = 0.6          # radius of the smallest crown kept [m]
CROWN_MAX = 6.0          # canopy further than this from a top belongs to no tree [m]
CROWN_PER_HEIGHT = 0.6   # a crown is no wider than this times the height of the tree
HEIGHT_MAX = 30.0        # higher than any tree on Texel: a mast or an error of the laser [m]


def polygonMask(rings, shape, transform):
    shapes = [{"type": "Polygon", "coordinates": [ring]} for ring in rings if len(ring) >= 4]
    if not shapes:
        return np.zeros(shape, dtype=bool)
    return rasterfeatures.rasterize(shapes, out_shape=shape, transform=transform, fill=0, default_value=1,
                                    all_touched=True).astype(bool)


def bgtBuildings():
    """Outlines of the buildings of the BGT as they are now"""
    rings = []
    for name in sorted(os.listdir(BGT)):
        if not (name.startswith("pand_") and name.endswith(".json")):
            continue
        for feature in json.load(open(os.path.join(BGT, name))).get("features", []):
            if (feature.get("properties") or {}).get("eind_registratie") is not None:
                continue
            geometry = feature.get("geometry") or {}
            polygons = geometry.get("coordinates", [])
            if geometry.get("type") == "Polygon":
                polygons = [polygons]
            rings += [[(p[0], p[1]) for p in polygon[0]] for polygon in polygons if polygon]
    return rings


def houseOutlines(margin):
    """The rectangles of the houses and tents, a margin wider"""
    rings = []
    for house in json.load(open(HOUSES))["houses"]:
        azimuth = math.radians(house["axis_azimuth"])
        along = (math.sin(azimuth), math.cos(azimuth))
        across = (along[1], -along[0])
        halfLength = 0.5 * house["length"] + margin
        halfWidth = 0.5 * house["width"] + margin
        corners = [(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0), (-1.0, -1.0)]
        rings.append([(house["x"] + u * halfLength * along[0] + v * halfWidth * across[0],
                       house["y"] + u * halfLength * along[1] + v * halfWidth * across[1]) for u, v in corners])
    return rings


def canopyHeight(threshold):
    """Height above the ground of what is no building and no water; 0 where there is nothing"""
    with rasterio.open(SURFACE) as source:
        band = source.read(1, masked=True)
        surface = band.filled(np.nan).astype(np.float32)
        transform = source.transform
    with rasterio.open(GROUND) as source:
        ground = source.read(1)
    with rasterio.open(CLASSES) as source:
        classes = source.read(1)
    cell = transform.a
    margin = int(round(BUILDING_MARGIN / cell))
    known = polygonMask(bgtBuildings(), surface.shape, transform)
    known |= polygonMask(houseOutlines(HOUSE_MARGIN), surface.shape, transform)

    # The laser does not reach the ground under a roof, nor under a dense crown: both are 'building' to
    # tools/make_terrain.py. A roof is a known building, or it is smooth; the rest is canopy.
    filled = np.where(np.isfinite(surface), surface, ground)
    rough = np.abs(filled - ndimage.uniform_filter(filled, size=3))
    gaps, count = ndimage.label(classes == BUILDING)
    index = np.arange(1, count + 1)
    isRoof = (ndimage.maximum(known, gaps, index) > 0) | (ndimage.median(rough, gaps, index) < ROOF_ROUGHNESS)
    roofs = np.concatenate([[False], isRoof])[gaps]
    excluded = ndimage.binary_dilation(roofs | known, iterations=margin) | (classes == WATER) | (classes == SEA)
    height = surface - ground
    height[~np.isfinite(height) | excluded | (height < threshold) | (height > HEIGHT_MAX)] = 0.0
    return height, ground, excluded, transform


def findTrees(height, ground, transform, threshold):
    """One tree for every top of the canopy: position, height and the radius of its crown"""
    cell = transform.a
    canopy = height > 0.0
    smooth = ndimage.gaussian_filter(height, SMOOTH / cell)
    window = 2 * int(round(TOP_DISTANCE / cell)) + 1
    tops = canopy & (smooth == ndimage.maximum_filter(smooth, size=window)) & (smooth >= 0.5 * threshold)
    topLabels, topCount = ndimage.label(tops)   # a flat top is several cells: one tree
    if topCount == 0:
        return []
    centres = np.array(ndimage.center_of_mass(tops, topLabels, range(1, topCount + 1)))

    # Every cell of the canopy goes to the nearest top
    rows, columns = np.nonzero(canopy)
    distance, nearest = cKDTree(centres).query(np.column_stack([rows, columns]),
                                               distance_upper_bound=CROWN_MAX / cell)
    reached = np.isfinite(distance)
    cells = np.bincount(nearest[reached], minlength=topCount)
    highest = ndimage.maximum(height, topLabels, range(1, topCount + 1))
    best = np.zeros(topCount)
    np.maximum.at(best, nearest[reached], height[rows[reached], columns[reached]])

    trees = []
    for index in range(topCount):
        treeHeight = float(max(highest[index], best[index]))
        radius = min(math.sqrt(cells[index] * cell * cell / math.pi), CROWN_PER_HEIGHT * treeHeight)
        if treeHeight < threshold or radius < CROWN_MIN:
            continue
        row, column = centres[index]
        x, y = transform * (column + 0.5, row + 0.5)
        base = float(ground[int(round(row)), int(round(column))])
        trees.append({"x": round(x, 2), "y": round(y, 2), "ground": round(base, 2),
                      "height": round(treeHeight, 2), "crown_radius": round(radius, 2)})
    return trees


def preview(height, excluded, transform, trees):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import Circle

    os.makedirs(PREVIEW, exist_ok=True)
    rows, columns = height.shape
    west, north = transform * (0, 0)
    east, south = transform * (columns, rows)
    figure, axis = plt.subplots(figsize=(28, 20), dpi=110)
    shown = np.ma.masked_where(height <= 0.0, height)
    axis.imshow(np.where(excluded, 0.82, 1.0), cmap="gray", vmin=0.0, vmax=1.0, extent=(west, east, south, north))
    image = axis.imshow(shown, cmap="YlGn", vmin=0.0, vmax=15.0, extent=(west, east, south, north))
    for tree in trees:
        axis.add_patch(Circle((tree["x"], tree["y"]), tree["crown_radius"], fill=False, edgecolor="#b3261e",
                              linewidth=0.5))
    axis.set_title("Trees from the AHN: %d (green: height above the ground, grey: buildings and water left out, "
                   "red: crown of a tree)" % len(trees))
    axis.set_xlabel("RD east [m]")
    axis.set_ylabel("RD north [m]")
    figure.colorbar(image, ax=axis, shrink=0.6, label="height above the ground [m]")
    figure.savefig(os.path.join(PREVIEW, "trees.png"), bbox_inches="tight")
    plt.close(figure)

    # The middle of the park, large enough to see single trees
    figure, axis = plt.subplots(figsize=(20, 20), dpi=110)
    axis.imshow(np.where(excluded, 0.82, 1.0), cmap="gray", vmin=0.0, vmax=1.0, extent=(west, east, south, north))
    axis.imshow(shown, cmap="YlGn", vmin=0.0, vmax=15.0, extent=(west, east, south, north))
    for tree in trees:
        axis.add_patch(Circle((tree["x"], tree["y"]), tree["crown_radius"], fill=False, edgecolor="#b3261e",
                              linewidth=0.8))
    axis.set_xlim(118600.0, 118900.0)
    axis.set_ylim(574700.0, 575000.0)
    axis.set_title("Detail around the Reception")
    figure.savefig(os.path.join(PREVIEW, "trees_detail.png"), bbox_inches="tight")
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description="Finds the trees of the park in the AHN")
    parser.add_argument("--threshold", type=float, default=THRESHOLD,
                        help="lower than this above the ground is not a tree [m] (default %.1f)" % THRESHOLD)
    parser.add_argument("--species", default="scots_pine", help="the tree drawn for all of them")
    parser.add_argument("--no-preview", action="store_true")
    arguments = parser.parse_args()
    if arguments.threshold <= 0.0:
        parser.error("the threshold must be above 0")
    if not os.path.exists(GROUND):
        parser.error("run tools/make_terrain.py first: %s is missing" % os.path.relpath(GROUND, ROOT))

    height, ground, excluded, transform = canopyHeight(arguments.threshold)
    trees = findTrees(height, ground, transform, arguments.threshold)
    trees.sort(key=lambda tree: (tree["y"], tree["x"]))
    heights = [tree["height"] for tree in trees]
    data = {
        "note": "Made by tools/make_trees.py from the AHN; do not edit by hand. x, y: RD New [m]; ground [m NAP]; "
                "height above the ground [m]; crown_radius [m].",
        "source": {"surface": os.path.relpath(SURFACE, ROOT), "ground": "tools/make_terrain.py (AHN ground model)",
                   "left_out": "buildings of the AHN and the BGT, houses and tents of data/houses.json, water"},
        "threshold_m": arguments.threshold,
        "species": arguments.species,
        "trees": trees,
    }
    with open(OUT, "w") as out:
        out.write(json.dumps(data, indent=None, separators=(",", ":")).replace('{"x"', '\n{"x"') + "\n")
    print("%d trees, threshold %.2f m; height median %.1f m, highest %.1f m -> %s" % (
        len(trees), arguments.threshold, float(np.median(heights)) if heights else 0.0,
        max(heights) if heights else 0.0, os.path.relpath(OUT, ROOT)))
    if not arguments.no_preview:
        preview(height, excluded, transform, trees)
        print("preview -> %s" % os.path.relpath(PREVIEW, ROOT))


if __name__ == "__main__":
    main()
