#!/usr/bin/env python3
# Slufter3D - checks that the files in data/ agree with each other and with models/
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/check_data.py     (also run by make test)
import json
import os
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def main():
    failures = []

    def check(condition, what):
        print("  %s  %s" % ("ok   " if condition else "FAIL ", what))
        if not condition:
            failures.append(what)

    types = json.load(open(os.path.join(ROOT, "data", "type_models.json")))["types"]
    data = json.load(open(os.path.join(ROOT, "data", "houses.json")))
    houses = data["houses"]

    # Every model named in the mapping has its file, and the other way round
    named = set()
    for entry in types.values():
        named.add(entry["model"])
        named.add(entry.get("semi_model", entry["model"]))
    files = {f[:-4] for f in os.listdir(os.path.join(ROOT, "models"))
             if f.endswith(".cpp") and "const Model& Models::" in open(os.path.join(ROOT, "models", f)).read()}
    check(named <= files, "every model in type_models.json has a file in models/: %s" % (sorted(named - files) or "all"))
    check(files <= named, "every model in models/ is used by a type: %s" % (sorted(files - named) or "all"))
    check(all(entry["size"] in ("model", "outline") for entry in types.values()), "size is 'model' or 'outline'")

    # Every house has a type with a model, a place and a front
    unknown = sorted({h["type"] for h in houses} - set(types))
    check(not unknown, "every type in houses.json has a model: %s" % (unknown or "all"))
    check(all(not h.get("semi_with") or "semi_model" in types.get(h["type"], {}) for h in houses),
          "every half of a block has a type with a semi_model")
    numbers = [h["nr"] for h in houses if h["nr"]]
    check(len(numbers) == len(set(numbers)), "house numbers are unique (%d)" % len(numbers))
    check(all(abs((h["front_azimuth"] - h["axis_azimuth"]) % 180.0 - 90.0) < 0.2 for h in houses),
          "every front faces square to the long axis")
    byNumber = {h["nr"]: h for h in houses if h["nr"]}
    check(all(byNumber[h["semi_with"]].get("semi_with") == h["nr"] for h in houses if h.get("semi_with")),
          "the halves of a block name each other")
    bounds = (118200, 574500, 119600, 575500)   # of raw/ahn/park_*.tif
    check(all(bounds[0] < h["x"] < bounds[2] and bounds[1] < h["y"] < bounds[3] for h in houses),
          "every house lies within the park rasters")
    check(data["central_building"] is not None, "the central building was found")

    print("All checks passed" if not failures else "Checks failed: %d" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
