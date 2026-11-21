#!/usr/bin/env python3
# Slufter3D - checks the buildings of the old park against the photographs: where was each one taken from?
# Author: Jan de Kort
# Created: 2026-11-21
#
# Usage: python3 tools/check_old_park_photos.py
#
# data/old_park_photos.json says in which column of a photograph the lookout tower and a number of buildings
# stand, and which building of data/old_park.json each is taken to be. A photograph fixes the angles between
# them. This tool finds the place, the direction and the focal length of the camera that fit those angles best,
# and prints how far each building lies beside its line of sight. A name or a type given to a building in
# data/old_park_corrections.json rests on these: metres beside the line mean the wrong building.
#
# The camera is a pinhole: a column x stands atan((x - middle) / f) beside the direction of the camera, with the
# middle that of the frame of the slide and f in pixels (36 mm of film is the width of the frame).
#
# Needs: numpy, scipy
import json
import math
import os

import numpy as np
from scipy.optimize import least_squares

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PHOTOS = os.path.join(ROOT, "data", "old_park_photos.json")
OLD_PARK = os.path.join(ROOT, "data", "old_park.json")
LANDMARKS = os.path.join(ROOT, "data", "landmarks.json")
SAME = 10.0     # a sight belongs to the building of the old park within this of its position [m]


def bearing(camera, point):
    """Azimuth from the camera to a point [rad], clockwise from north"""
    return math.atan2(point[0] - camera[0], point[1] - camera[1])


def main():
    buildings = json.load(open(OLD_PARK))["buildings"]
    tower = json.load(open(LANDMARKS))["lookout_tower"]["rd"]
    for photo in json.load(open(PHOTOS))["photos"]:
        middle = 0.5 * (photo["frame"][0] + photo["frame"][1])
        width = photo["frame"][1] - photo["frame"][0]
        targets = [("the tower", photo["tower_px"], tower)] if photo["tower_px"] is not None else []
        for sight in photo["sights"]:
            near = min(buildings, key=lambda b: math.hypot(b["x"] - sight["building"][0], b["y"] - sight["building"][1]))
            if math.hypot(near["x"] - sight["building"][0], near["y"] - sight["building"][1]) > SAME:
                raise SystemExit("%s: no building at %s" % (photo["file"], sight["building"]))
            targets.append(("%s = %s %s" % (sight["what"], near["id"], near["name"]), sight["x_px"], (near["x"], near["y"])))

        def beside(p):
            """How far each target lies beside its line of sight [m]; p: east, north, heading [rad], focal length [mm]"""
            f = p[3] / 36.0 * width
            out = []
            for _, x, point in targets:
                angle = p[2] + math.atan((x - middle) / f) - bearing(p, point)
                angle = (angle + math.pi) % (2.0 * math.pi) - math.pi
                out.append(angle * math.hypot(point[0] - p[0], point[1] - p[1]))
            return out

        first = targets[len(targets) // 2]
        best = None
        for focal in (28.0, 35.0, 50.0, 70.0, 100.0):
            start = [photo["start"][0], photo["start"][1], bearing(photo["start"], first[2]), focal]
            fit = least_squares(beside, start, bounds=([-np.inf, -np.inf, -np.inf, 24.0], [np.inf, np.inf, np.inf, 200.0]))
            if best is None or fit.cost < best.cost:
                best = fit
        p = best.x
        print("%s\n   taken at RD %.0f, %.0f, looking %.0f deg, lens %.0f mm" % (photo["file"], p[0], p[1], math.degrees(p[2]) % 360.0, p[3]))
        for (what, _, point), metres in zip(targets, beside(p)):
            print("   %5.1f m beside the line, %4.0f m away: %s" % (metres, math.hypot(point[0] - p[0], point[1] - p[1]), what))


if __name__ == "__main__":
    main()
