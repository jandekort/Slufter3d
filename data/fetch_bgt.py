#!/usr/bin/env python3
# Slufter3D - downloads the large-scale topography (BGT) of the park from PDOK, once, into raw/bgt/,
# and draws a preview to check what it holds
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/fetch_bgt.py                  download what is missing, then draw the preview
#        python3 tools/fetch_bgt.py --dry-run        show the requests; fetch nothing
#        python3 tools/fetch_bgt.py --preview-only   draw the preview from what is in raw/bgt/
#        python3 tools/fetch_bgt.py --verify         check the files in raw/bgt/ against their checksums
#
# The BGT is the national map of the Netherlands at the scale of single roads and buildings. Its road parts
# say how they are paved: closed (asphalt), open (bricks), half paved (shells, gravel) or unpaved.
#
# Raw data is written once and never changed, as in tools/fetch_ahn.py. The service gives each kind of
# object (a collection) in pages; the body of every page is stored byte for byte as <collection>_<page>.json,
# and <collection>.json records the requests, the date and the SHA-256 of every page. A collection that
# is complete is skipped, never replaced, and every file is made read-only.
# The preview goes to build/preview/bgt/; it is a check, not data.
#
# Licence of the BGT: CC BY 4.0, source Kadaster / the BGT source holders. Name the source where the data is shown.
#
# Needs: matplotlib for the preview (pip install matplotlib)
import argparse
import datetime
import hashlib
import json
import os
import stat
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RAW_DIR = os.path.join(ROOT, "raw", "bgt")
PREVIEW_DIR = os.path.join(ROOT, "build", "preview", "bgt")

SERVICE_URL = "https://api.pdok.nl/lv/bgt/ogc/v1"
RD_NEW = "http://www.opengis.net/def/crs/EPSG/0/28992"
PAGE = 1000              # objects asked for per page; the service may give fewer
MAX_PAGES = 200          # a collection of the park is far smaller: more means something is wrong
TIMEOUT = 300.0          # per request [s]
RETRIES = 3
USER_AGENT = "slufter3d-fetch-bgt/1.0"

SOURCE = {
    "dataset": "Basisregistratie Grootschalige Topografie (BGT), as served by the PDOK OGC API Features",
    "owner": "Kadaster and the BGT source holders",
    "licence": "CC BY 4.0",
    "licence_url": "https://creativecommons.org/licenses/by/4.0/",
    "catalogue": "https://data.overheid.nl/dataset/43122-basisregistratie--grootschalige-topografie--bgt---donl-",
}

# Rectangle in RD New [m]: west, south, east, north; that of the park rasters of tools/fetch_ahn.py
BOUNDS = (118200, 574500, 119600, 575500)

# Collections to fetch, and why
COLLECTIONS = {
    "wegdeel": "road parts with their kind of paving: the main road, the shell roads, parking",
    "ondersteunendwegdeel": "verges and other parts of the road that are not driven on",
    "waterdeel": "ponds and ditches",
    "begroeidterreindeel": "ground with vegetation: dune, grass, shrubs, wood",
    "onbegroeidterreindeel": "ground without vegetation: yards, sand, paved terrain",
    "pand": "buildings, to compare with the AHN",
}

# Preview colours by the paving of a road part (Dutch values of the BGT, lower case)
PAVING = [
    ("gesloten verharding", "#555555", "closed paving (asphalt)"),
    ("open verharding", "#a0522d", "open paving (bricks)"),
    ("half verhard", "#f2d16b", "half paved (shells, gravel)"),
    ("onverhard", "#c8b68a", "unpaved"),
]
FILL = {"waterdeel": "#6fa8dc", "begroeidterreindeel": "#cfe8c0", "onbegroeidterreindeel": "#ece6d8",
        "ondersteunendwegdeel": "#dddddd", "pand": "#b07aa1"}


# ---------------------------------------------------------------- requests

def firstUrl(service, collection):
    west, south, east, north = BOUNDS
    query = {"f": "json", "limit": PAGE, "bbox": "%d,%d,%d,%d" % (west, south, east, north),
             "bbox-crs": RD_NEW, "crs": RD_NEW}
    return "%s/collections/%s/items?%s" % (service, collection, urllib.parse.urlencode(query))


def fetch(url):
    """The body of the response to a GET request, retried on network errors"""
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "application/geo+json"})
    for attempt in range(1, RETRIES + 1):
        try:
            with urllib.request.urlopen(request, timeout=TIMEOUT) as response:
                return response.read()
        except urllib.error.HTTPError as error:
            body = error.read()
            raise RuntimeError("HTTP %d from %s\n%s" % (error.code, url, body[:2000].decode("utf-8", "replace")))
        except (urllib.error.URLError, TimeoutError, ConnectionError) as error:
            if attempt == RETRIES:
                raise RuntimeError("Could not reach %s after %d attempts: %s" % (url, RETRIES, error))
            print("    network error (%s), retrying in %d s" % (error, 5 * attempt))
            time.sleep(5 * attempt)
    raise RuntimeError("unreachable")


def nextUrl(page):
    for link in page.get("links", []):
        if link.get("rel") == "next" and link.get("href"):
            return link["href"]
    return None


def inRdNew(page):
    """true->the coordinates of the page are metres in RD New, not degrees"""
    for feature in page.get("features", []):
        geometry = feature.get("geometry") or {}
        point = geometry.get("coordinates")
        while isinstance(point, list) and point and isinstance(point[0], list):
            point = point[0]
        if isinstance(point, list) and len(point) >= 2:
            return abs(point[0]) > 1000.0 and abs(point[1]) > 1000.0
    return True


# ---------------------------------------------------------------- raw files: written once

def writeOnce(path, body):
    if os.path.exists(path):
        raise RuntimeError("%s exists; raw files are never replaced" % path)
    partial = path + ".part"
    with open(partial, "wb") as handle:
        handle.write(body)
    os.replace(partial, path)
    os.chmod(path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def download(collection, service, rawDir):
    record = os.path.join(rawDir, collection + ".json")
    if os.path.exists(record):
        print("  %-24s already there, kept as it is" % collection)
        return False
    leftovers = [f for f in (os.listdir(rawDir) if os.path.isdir(rawDir) else []) if f.startswith(collection + "_")]
    if leftovers:
        raise RuntimeError("raw/bgt holds pages of %s without their record (%s): an earlier run stopped half way."
                           " Remove these pages by hand to fetch the collection again." % (collection, ", ".join(leftovers)))
    # All pages are fetched and checked before any of them becomes raw data
    pages = []
    url = firstUrl(service, collection)
    while url:
        if len(pages) >= MAX_PAGES:
            raise RuntimeError("%s has more than %d pages; something is wrong with the request" % (collection, MAX_PAGES))
        print("  %-24s page %d: %s" % (collection, len(pages) + 1, url))
        body = fetch(url)
        try:
            page = json.loads(body)
        except ValueError:
            raise RuntimeError("The service did not answer with JSON:\n%s" % body[:2000].decode("utf-8", "replace"))
        if page.get("type") != "FeatureCollection":
            raise RuntimeError("The service did not answer with features:\n%s" % json.dumps(page, indent=1)[:2000])
        if not inRdNew(page):
            raise RuntimeError("The coordinates of %s are not in RD New; the service ignored crs=%s" % (collection, RD_NEW))
        pages.append({"url": url, "body": body, "features": len(page.get("features", []))})
        url = nextUrl(page)

    os.makedirs(rawDir, exist_ok=True)
    listed = []
    for number, page in enumerate(pages, 1):
        name = "%s_%03d.json" % (collection, number)
        writeOnce(os.path.join(rawDir, name), page["body"])
        listed.append({"file": name, "request": page["url"], "features": page["features"], "bytes": len(page["body"]),
                       "sha256": sha256(os.path.join(rawDir, name))})
    summary = {
        "collection": collection,
        "why": COLLECTIONS[collection],
        "source": SOURCE,
        "bounds_rd": list(BOUNDS),
        "crs": RD_NEW,
        "downloaded_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "features": sum(page["features"] for page in pages),
        "pages": listed,
        "note": "The bodies of the responses of the service, stored byte for byte. Objects that cross the edge of"
                " the rectangle are given whole. Never edit these files; derived data goes elsewhere.",
    }
    writeOnce(record, (json.dumps(summary, indent=2) + "\n").encode("utf-8"))
    print("    stored %d objects in %d page(s)" % (summary["features"], len(pages)))
    return True


def records(rawDir):
    found = []
    if os.path.isdir(rawDir):
        for collection in COLLECTIONS:
            path = os.path.join(rawDir, collection + ".json")
            if os.path.exists(path):
                found.append(json.load(open(path)))
    return found


def verify(rawDir):
    found = records(rawDir)
    if not found:
        print("Nothing to verify in %s" % rawDir)
        return True
    good = True
    for record in found:
        for page in record["pages"]:
            path = os.path.join(rawDir, page["file"])
            state = "MISSING" if not os.path.exists(path) else ("ok" if sha256(path) == page["sha256"] else "CHANGED")
            good = good and state == "ok"
            print("  %-9s %s" % (state, page["file"]))
    return good


# ---------------------------------------------------------------- preview

def features(rawDir, record):
    for page in record["pages"]:
        for feature in json.load(open(os.path.join(rawDir, page["file"]))).get("features", []):
            yield feature


def rings(geometry):
    """Outer rings of a polygon or multipolygon; lines as they are"""
    kind = (geometry or {}).get("type")
    coordinates = (geometry or {}).get("coordinates", [])
    if kind == "Polygon":
        return [coordinates[0]] if coordinates else []
    if kind == "MultiPolygon":
        return [polygon[0] for polygon in coordinates if polygon]
    if kind == "LineString":
        return [coordinates]
    if kind == "MultiLineString":
        return list(coordinates)
    return []


def property_(feature, part):
    """The value of the first property whose name holds part, lower case; '' when there is none"""
    for key, value in (feature.get("properties") or {}).items():
        if part in key.lower().replace("_", "").replace("-", "") and "plus" not in key.lower() and isinstance(value, str):
            return value.lower()
    return ""


def drawPreview(rawDir, outDir):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.patches import Patch, Polygon
    except ImportError:
        raise RuntimeError("The preview needs matplotlib: pip install matplotlib")
    found = {record["collection"]: record for record in records(rawDir)}
    if not found:
        print("No collections in %s yet" % rawDir)
        return None
    west, south, east, north = BOUNDS
    figure, axis = plt.subplots(figsize=(14.0, 14.0 * (north - south) / (east - west) * 0.95), dpi=160)
    axis.set_facecolor("#f7f7f2")
    counts = {}
    for collection in ("onbegroeidterreindeel", "begroeidterreindeel", "waterdeel", "ondersteunendwegdeel", "pand"):
        if collection in found:
            for feature in features(rawDir, found[collection]):
                for ring in rings(feature.get("geometry")):
                    axis.add_patch(Polygon([p[:2] for p in ring], closed=True, facecolor=FILL[collection],
                                           edgecolor="none"))
    if "wegdeel" in found:
        for feature in features(rawDir, found["wegdeel"]):
            paving = property_(feature, "fysiekvoorkomen")
            colour = next((c for value, c, _ in PAVING if value in paving), "#ff00ff")
            counts[paving or "(no paving given)"] = counts.get(paving or "(no paving given)", 0) + 1
            for ring in rings(feature.get("geometry")):
                axis.add_patch(Polygon([p[:2] for p in ring], closed=True, facecolor=colour, edgecolor="black",
                                       linewidth=0.15))
    # The houses of data/houses.json on top, when they are there
    housesPath = os.path.join(ROOT, "data", "houses.json")
    if os.path.exists(housesPath):
        import math
        for house in json.load(open(housesPath))["houses"]:
            a = math.radians(house["axis_azimuth"])
            along = (math.sin(a), math.cos(a))
            across = (math.cos(a), -math.sin(a))
            ring = [(house["x"] + su * 0.5 * house["length"] * along[0] + sv * 0.5 * house["width"] * across[0],
                     house["y"] + su * 0.5 * house["length"] * along[1] + sv * 0.5 * house["width"] * across[1])
                    for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            axis.add_patch(Polygon(ring, closed=True, facecolor="none", edgecolor="#1f4e9c", linewidth=0.5))
            if house["nr"]:
                axis.text(house["x"], house["y"], str(house["nr"]), fontsize=2.5, ha="center", va="center", color="#1f4e9c")
    axis.set_xlim(west, east)
    axis.set_ylim(south, north)
    axis.set_aspect("equal")
    axis.set_xlabel("RD x (east) [m]")
    axis.set_ylabel("RD y (north) [m]")
    legend = [Patch(facecolor=c, edgecolor="black", label=label) for _, c, label in PAVING]
    legend += [Patch(facecolor="#ff00ff", label="road part, paving not recognised"),
               Patch(facecolor=FILL["waterdeel"], label="water"), Patch(facecolor=FILL["begroeidterreindeel"], label="vegetation"),
               Patch(facecolor=FILL["pand"], label="building (BGT)"),
               Patch(facecolor="none", edgecolor="#1f4e9c", label="house (data/houses.json)")]
    axis.legend(handles=legend, loc="lower right", fontsize=7)
    axis.set_title("BGT of the park: %s. Source: BGT, Kadaster (CC BY 4.0)"
                   % ", ".join("%s %d" % (name, record["features"]) for name, record in found.items()), fontsize=8)
    os.makedirs(outDir, exist_ok=True)
    out = os.path.join(outDir, "bgt.png")
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    if counts:
        print("  road parts by paving: %s" % ", ".join("%s %d" % item for item in sorted(counts.items())))
    return out


# ---------------------------------------------------------------- main

def main():
    parser = argparse.ArgumentParser(description="Fetch the BGT of the park from PDOK into raw/bgt/, once.")
    parser.add_argument("--dry-run", action="store_true", help="show the requests; fetch nothing")
    parser.add_argument("--preview-only", action="store_true", help="only draw the preview of what is in raw/bgt/")
    parser.add_argument("--verify", action="store_true", help="check the raw files against their checksums")
    parser.add_argument("--only", nargs="+", metavar="COLLECTION", help="fetch only these")
    parser.add_argument("--service", default=SERVICE_URL, help="address of the API (default: %(default)s)")
    parser.add_argument("--raw-dir", default=RAW_DIR)
    parser.add_argument("--preview-dir", default=PREVIEW_DIR)
    parser.add_argument("--no-preview", action="store_true")
    arguments = parser.parse_args()

    if arguments.verify:
        return 0 if verify(arguments.raw_dir) else 1
    chosen = list(COLLECTIONS)
    if arguments.only:
        unknown = sorted(set(arguments.only) - set(COLLECTIONS))
        if unknown:
            parser.error("unknown collections %s; choose from %s" % (", ".join(unknown), ", ".join(COLLECTIONS)))
        chosen = [c for c in COLLECTIONS if c in arguments.only]
    if not arguments.preview_only:
        print("Area (RD New, west south east north): %d %d %d %d" % BOUNDS)
        for collection in chosen:
            if arguments.dry_run:
                print("  %-24s %s\n    %s" % (collection, COLLECTIONS[collection], firstUrl(arguments.service, collection)))
            else:
                download(collection, arguments.service, arguments.raw_dir)
    if not arguments.no_preview and not arguments.dry_run:
        out = drawPreview(arguments.raw_dir, arguments.preview_dir)
        if out:
            print("  preview %s" % out)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, ValueError) as error:
        print("\nStopped: %s" % error, file=sys.stderr)
        sys.exit(1)
