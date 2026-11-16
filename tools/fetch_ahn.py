#!/usr/bin/env python3
# Slufter - downloads the AHN height models of the park and its surroundings from PDOK, once,
# into raw/ahn/, and draws previews to check that they cover the right area
# Author: Jan de Kort
# Created: 2026-11-16
#
# Usage: python3 tools/fetch_ahn.py                  download what is missing, then draw the previews
#        python3 tools/fetch_ahn.py --dry-run        show the requests and draw the planned areas only
#        python3 tools/fetch_ahn.py --preview-only   draw the previews from what is in raw/ahn/
#        python3 tools/fetch_ahn.py --verify         check the files in raw/ahn/ against their checksums
#        python3 tools/fetch_ahn.py --only park_dtm_05m tip_dtm_2m
#
# Raw data is written once and never changed. A file that exists is skipped, never replaced, and every
# file is made read-only. The body of each WCS response is stored byte for byte as <name>.tif, next to
# <name>.tif.json with where, when and how it was fetched and its SHA-256. The service's own description
# (GetCapabilities, DescribeCoverage) is kept in raw/ahn/service/ with the date of the download.
# The previews go to build/preview/ahn/; they are checks, not data.
#
# Needs: numpy, rasterio, matplotlib (pip install numpy rasterio matplotlib)
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
import xml.etree.ElementTree as ElementTree

import numpy as np
import rasterio

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RAW_DIR = os.path.join(ROOT, "raw", "ahn")
PREVIEW_DIR = os.path.join(ROOT, "build", "preview", "ahn")

SERVICE_URL = "https://service.pdok.nl/rws/ahn/wcs/v1_0"
WCS_VERSION = "1.0.0"    # BBOX, WIDTH and HEIGHT: one request per file, on the AHN grid
NATIVE_CELL = 0.5        # cell of the AHN rasters [m]
MAX_SIDE = 4000          # largest request this script makes [cells per side]
RD_NEW = 28992           # EPSG code of Amersfoort / RD New
TIMEOUT = 600.0          # per request [s]
RETRIES = 3
USER_AGENT = "slufter-fetch-ahn/1.0"

SOURCE = {
    "dataset": "Actueel Hoogtebestand Nederland (AHN), 0.5 m rasters, as served by the PDOK WCS",
    "owner": "Rijkswaterstaat",
    "licence": "CC0 1.0",
    "licence_url": "https://creativecommons.org/publicdomain/zero/1.0/",
    "landing_page": "https://www.ahn.nl",
    "catalogue": {
        "dtm_05m": "https://data.overheid.nl/dataset/47567-actueel-hoogtebestand-nederland--ahn--dtm",
        "dsm_05m": "https://data.overheid.nl/dataset/36461-actueel-hoogtebestand-nederland-dsm--ahn4-",
    },
}

# Rectangles in RD New [m]: west, south, east, north. Whole metres, so their edges lie on the AHN grid.
AREAS = {
    "park": {
        "bounds": (118200, 574500, 119600, 575500),
        "why": "Landal Sluftervallei with a margin: the ground that is walked on, and the surface"
               " (houses, trees) to place the survey and the vegetation on.",
    },
    "tip": {
        "bounds": (114500, 574400, 119900, 578000),
        "why": "From the park west to the North Sea and north past the lighthouse: what is seen close by"
               " and flown over. The coast at the park's latitude lies near x = 115 km; the west edge"
               " leaves room for beach and surf.",
    },
    "horizon": {
        "bounds": (113000, 570500, 123000, 580000),
        "why": "The northern part of Texel with the North Sea and the Wadden Sea around it:"
               " the land on the horizon.",
    },
}

# Files to fetch: name, area, coverage, cell [m]. A cell larger than NATIVE_CELL is resampled by the service.
DOWNLOADS = [
    {"name": "park_dtm_05m", "area": "park", "coverage": "dtm_05m", "cell": 0.5},
    {"name": "park_dsm_05m", "area": "park", "coverage": "dsm_05m", "cell": 0.5},
    {"name": "tip_dtm_2m", "area": "tip", "coverage": "dtm_05m", "cell": 2.0},
    {"name": "tip_dsm_2m", "area": "tip", "coverage": "dsm_05m", "cell": 2.0},
    {"name": "horizon_dtm_5m", "area": "horizon", "coverage": "dtm_05m", "cell": 5.0},
]

# Drawn on the previews when present; read only, never needed for the download
OVERLAY_MAP = os.path.join(ROOT, "data", "sluftervallei_map.geojson")
OVERLAY_SCENERY = os.path.join(ROOT, "data", "scenery.json")
OVERLAY_DEM = os.path.join(ROOT, "data", "dem_1m.tif")
OVERLAY_HEIGHTMAP = os.path.join(ROOT, "data", "surroundings.json")

AREA_COLOURS = {"park": "#d62728", "tip": "#1f4e9c", "horizon": "#222222"}
NODATA_COLOUR = (0.80, 0.88, 0.94)


# ---------------------------------------------------------------- requests

def gridSize(download):
    west, south, east, north = AREAS[download["area"]]["bounds"]
    cell = download["cell"]
    width = int(round((east - west) / cell))
    height = int(round((north - south) / cell))
    if abs(width * cell - (east - west)) > 1e-6 or abs(height * cell - (north - south)) > 1e-6:
        raise ValueError("Area %s is not a whole number of %g m cells" % (download["area"], cell))
    if max(width, height) > MAX_SIDE:
        raise ValueError("%s would be %d x %d cells; split the area or use a larger cell (at most %d per side)"
                         % (download["name"], width, height, MAX_SIDE))
    return width, height


def serviceUrl(query, serviceUrl=None):
    return (serviceUrl or SERVICE_URL) + "?" + urllib.parse.urlencode(query)


def coverageUrl(download, outputFormat, interpolation, service):
    west, south, east, north = AREAS[download["area"]]["bounds"]
    width, height = gridSize(download)
    query = {
        "SERVICE": "WCS",
        "VERSION": WCS_VERSION,
        "REQUEST": "GetCoverage",
        "COVERAGE": download["coverage"],
        "CRS": "EPSG:%d" % RD_NEW,
        "RESPONSE_CRS": "EPSG:%d" % RD_NEW,
        "BBOX": "%d,%d,%d,%d" % (west, south, east, north),
        "WIDTH": width,
        "HEIGHT": height,
        "FORMAT": outputFormat,
    }
    if interpolation:
        query["INTERPOLATION"] = interpolation
    return serviceUrl(query, service)


def fetch(url):
    """The body of the response to a GET request, retried on network errors"""
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
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


def isTiff(body):
    return body[:4] in (b"II*\x00", b"MM\x00*", b"II+\x00", b"MM\x00+")


def exceptionText(body):
    """The message of a WCS exception report, or the start of whatever came back"""
    text = body[:4000].decode("utf-8", "replace")
    try:
        root = ElementTree.fromstring(body)
        messages = [element.text.strip() for element in root.iter() if element.text and element.text.strip()]
        if messages:
            return " | ".join(messages)
    except ElementTree.ParseError:
        pass
    return text


# ---------------------------------------------------------------- the service's own description

def localName(element):
    return element.tag.rsplit("}", 1)[-1]


def describeCoverage(body):
    """Output formats, interpolations and null value from a WCS 1.0.0 DescribeCoverage response"""
    root = ElementTree.fromstring(body)
    formats = []
    interpolations = []
    nulls = []
    for element in root.iter():
        name = localName(element)
        text = (element.text or "").strip()
        if name == "formats" and text:
            formats.extend(text.split())
        elif name == "interpolationMethod" and text:
            interpolations.append(text)
        elif name in ("singleValue", "nullValue") and text:
            nulls.append(text)
    return {"formats": formats, "interpolations": interpolations, "null_values": nulls}


def chooseFormat(formats):
    tiffs = [f for f in formats if "tif" in f.lower()]
    if not tiffs:
        raise RuntimeError("The service offers no GeoTIFF output; it offers: %s" % ", ".join(formats))
    for preferred in ("float32", "float"):
        for f in tiffs:
            if preferred in f.lower():
                return f
    return tiffs[0]


def chooseInterpolations(offered):
    """Methods to try for a resampled request, best first; None leaves it to the service"""
    lowered = {method.lower(): method for method in offered}
    order = []
    for wanted in ("average", "bilinear"):
        for key, method in lowered.items():
            if wanted in key and method not in order:
                order.append(method)
    order.append(None)
    return order


def readServiceDescription(service, coverages, saveDir, today, dryRun):
    """GetCapabilities and DescribeCoverage for each coverage; kept in raw/ahn/service/ unless a dry run"""
    documents = {}
    capabilities = fetch(serviceUrl({"SERVICE": "WCS", "VERSION": WCS_VERSION, "REQUEST": "GetCapabilities"}, service))
    documents["GetCapabilities_%s.xml" % today] = capabilities
    offered = set()
    for element in ElementTree.fromstring(capabilities).iter():
        if localName(element) == "name" and element.text:
            offered.add(element.text.strip())
    described = {}
    for coverage in coverages:
        if coverage not in offered:
            raise RuntimeError("The service does not offer coverage %s; it offers: %s"
                               % (coverage, ", ".join(sorted(offered))))
        body = fetch(serviceUrl({"SERVICE": "WCS", "VERSION": WCS_VERSION, "REQUEST": "DescribeCoverage",
                                 "COVERAGE": coverage}, service))
        documents["DescribeCoverage_%s_%s.xml" % (coverage, today)] = body
        described[coverage] = describeCoverage(body)
    if not dryRun:
        os.makedirs(saveDir, exist_ok=True)
        for name, body in documents.items():
            path = os.path.join(saveDir, name)
            if not os.path.exists(path):
                writeOnce(path, body)
    return described


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


def rasterSummary(path):
    with rasterio.open(path) as source:
        data = source.read(1, masked=True)
        valid = data.compressed()
        return {
            "width": source.width,
            "height": source.height,
            "bounds": [round(v, 3) for v in source.bounds],
            "cell": [abs(source.transform.a), abs(source.transform.e)],
            "epsg": source.crs.to_epsg() if source.crs else None,
            "dtype": source.dtypes[0],
            "nodata": None if source.nodata is None else float(source.nodata),
            "nodata_fraction": round(1.0 - valid.size / data.size, 4),
            "min": None if valid.size == 0 else round(float(valid.min()), 3),
            "max": None if valid.size == 0 else round(float(valid.max()), 3),
            "mean": None if valid.size == 0 else round(float(valid.mean()), 3),
        }


def checkRaster(path, download):
    """Raises if the file is not the raster that was asked for"""
    west, south, east, north = AREAS[download["area"]]["bounds"]
    width, height = gridSize(download)
    cell = download["cell"]
    with rasterio.open(path) as source:
        problems = []
        if source.count != 1:
            problems.append("%d bands instead of 1" % source.count)
        if (source.width, source.height) != (width, height):
            problems.append("%d x %d cells instead of %d x %d" % (source.width, source.height, width, height))
        if source.crs is None:
            problems.append("no coordinate reference system")
        elif source.crs.to_epsg() != RD_NEW and "Amersfoort" not in source.crs.to_wkt():
            problems.append("coordinate reference system %s instead of RD New" % source.crs.to_string())
        for got, wanted, side in zip(source.bounds, (west, south, east, north), ("west", "south", "east", "north")):
            if abs(got - wanted) > 0.5 * cell:
                problems.append("%s edge at %.2f instead of %d" % (side, got, wanted))
        if problems:
            raise RuntimeError("%s is not what was asked for: %s" % (download["name"], "; ".join(problems)))


def download(item, described, service, rawDir, rejectDir):
    path = os.path.join(rawDir, item["name"] + ".tif")
    sidecar = path + ".json"
    if os.path.exists(path) or os.path.exists(sidecar):
        if not (os.path.exists(path) and os.path.exists(sidecar)):
            raise RuntimeError("%s is incomplete: the raster and its .json belong together. Remove what is"
                               " there by hand to fetch it again." % path)
        print("  %-16s already there, kept as it is" % item["name"])
        return False

    description = described[item["coverage"]]
    outputFormat = chooseFormat(description["formats"])
    resampled = item["cell"] > NATIVE_CELL
    interpolations = chooseInterpolations(description["interpolations"]) if resampled else [None]
    refused = []
    for interpolation in interpolations:
        url = coverageUrl(item, outputFormat, interpolation, service)
        print("  %-16s fetching %s" % (item["name"], url))
        started = time.time()
        body = fetch(url)
        if isTiff(body):
            break
        refused.append({"interpolation": interpolation, "answer": exceptionText(body)})
        print("    refused: %s" % refused[-1]["answer"][:300])
    else:
        raise RuntimeError("The service gave no GeoTIFF for %s:\n%s"
                           % (item["name"], json.dumps(refused, indent=2)))

    # Checked before it becomes raw data; a raster that is not what was asked for is set aside
    os.makedirs(rawDir, exist_ok=True)
    trial = path + ".check"
    with open(trial, "wb") as handle:
        handle.write(body)
    try:
        checkRaster(trial, item)
    except RuntimeError:
        os.makedirs(rejectDir, exist_ok=True)
        os.replace(trial, os.path.join(rejectDir, item["name"] + ".rejected.tif"))
        raise
    os.remove(trial)
    writeOnce(path, body)

    width, height = gridSize(item)
    record = {
        "name": item["name"],
        "file": os.path.basename(path),
        "source": dict(SOURCE, catalogue=SOURCE["catalogue"].get(item["coverage"])),
        "coverage": item["coverage"],
        "area": item["area"],
        "area_why": AREAS[item["area"]]["why"],
        "bounds_rd": list(AREAS[item["area"]]["bounds"]),
        "cell_m": item["cell"],
        "native_cell_m": NATIVE_CELL,
        "resampled_by_service": resampled,
        "interpolation_requested": (interpolation or "none: the service's default") if resampled
                                   else "none: native cells",
        "interpolations_refused": refused,
        "interpolations_offered": description["interpolations"],
        "null_values_described": description["null_values"],
        "grid": [width, height],
        "format": outputFormat,
        "request": url,
        "downloaded_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "seconds": round(time.time() - started, 1),
        "bytes": len(body),
        "sha256": sha256(path),
        "raster": rasterSummary(path),
        "note": "The body of the WCS response, stored byte for byte. Never edit this file or the raster;"
                " derived data goes elsewhere.",
    }
    writeOnce(sidecar, (json.dumps(record, indent=2) + "\n").encode("utf-8"))
    print("    stored %s: %.1f MB, %d x %d cells, heights %s to %s m, %.1f%% no data"
          % (os.path.basename(path), len(body) / 1e6, width, height, record["raster"]["min"],
             record["raster"]["max"], 100.0 * record["raster"]["nodata_fraction"]))
    return True


def verify(rawDir):
    names = sorted(os.listdir(rawDir)) if os.path.isdir(rawDir) else []
    sidecars = [f for f in names if f.endswith(".tif.json")]
    if not any(f.endswith(".tif") or f.endswith(".tif.json") for f in names):
        print("Nothing to verify in %s" % rawDir)
        return True
    good = True
    for name in names:
        if name.endswith(".tif") and name + ".json" not in sidecars:
            print("  NO RECORD %s" % name)
            good = False
    for name in sidecars:
        record = json.load(open(os.path.join(rawDir, name)))
        path = os.path.join(rawDir, record["file"])
        if not os.path.exists(path):
            print("  MISSING   %s" % record["file"])
            good = False
            continue
        same = sha256(path) == record["sha256"]
        good = good and same
        print("  %-9s %s" % ("ok" if same else "CHANGED", record["file"]))
    return good


# ---------------------------------------------------------------- previews

def loadOverlays():
    """Things to recognise on the previews, from the current data/ when it is there"""
    overlays = {"houses": [], "points": [], "rectangles": []}
    if os.path.exists(OVERLAY_MAP):
        survey = json.load(open(OVERLAY_MAP))
        for feature in survey["features"]:
            if feature["properties"].get("layer") == "building" and feature["geometry"]["type"] == "Polygon":
                overlays["houses"].append(np.array(feature["geometry"]["coordinates"][0])[:, :2])
        anchor = survey.get("metadata", {}).get("anchor", {}).get("rd")
        if anchor:
            overlays["points"].append(("park anchor (survey)", anchor[0], anchor[1], "+"))
    if os.path.exists(OVERLAY_SCENERY):
        for item in json.load(open(OVERLAY_SCENERY)).get("items", []):
            overlays["points"].append((item["name"], item["rd"][0], item["rd"][1], "*"))
    if os.path.exists(OVERLAY_DEM):
        with rasterio.open(OVERLAY_DEM) as source:
            overlays["rectangles"].append(("current dem_1m.tif", tuple(source.bounds)))
    return overlays


def colormap(name):
    import matplotlib
    return matplotlib.colormaps[name]


def pyplot():
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        return plt
    except ImportError:
        raise RuntimeError("Previews need matplotlib: pip install matplotlib")


def kilometres(axis):
    from matplotlib.ticker import FuncFormatter
    axis.xaxis.set_major_formatter(FuncFormatter(lambda v, _: "%.1f" % (v / 1000.0)))
    axis.yaxis.set_major_formatter(FuncFormatter(lambda v, _: "%.1f" % (v / 1000.0)))
    axis.set_xlabel("RD x (east) [km]")
    axis.set_ylabel("RD y (north) [km]")
    axis.grid(True, color="white", alpha=0.35, linewidth=0.5)


def drawOverlays(axis, overlays, bounds, showHouses):
    from matplotlib.patches import Rectangle
    west, south, east, north = bounds
    for name, area in AREAS.items():
        w, s, e, n = area["bounds"]
        axis.add_patch(Rectangle((w, s), e - w, n - s, fill=False, linewidth=1.6,
                                 edgecolor=AREA_COLOURS.get(name, "black")))
        if w >= west - 1 and n <= north + 1:
            axis.text(w + 0.01 * (east - west), n - 0.01 * (north - south), name, va="top", ha="left",
                      fontsize=9, color=AREA_COLOURS.get(name, "black"), fontweight="bold")
    for name, (w, s, e, n) in overlays["rectangles"]:
        axis.add_patch(Rectangle((w, s), e - w, n - s, fill=False, linewidth=1.0, linestyle="--",
                                 edgecolor="#ff7f0e"))
        axis.text(w, s, " " + name, va="bottom", ha="left", fontsize=7, color="#ff7f0e")
    if showHouses:
        for ring in overlays["houses"]:
            axis.plot(ring[:, 0], ring[:, 1], color="black", linewidth=0.5)
    for name, x, y, marker in overlays["points"]:
        if west <= x <= east and south <= y <= north:
            axis.plot([x], [y], marker=marker, color="#d62728", markersize=12 if marker == "*" else 10,
                      markeredgecolor="white" if marker == "*" else "#d62728", linestyle="none")
            axis.annotate(name, (x, y), textcoords="offset points", xytext=(8, 6), fontsize=8,
                          color="#d62728", fontweight="bold")
    axis.set_xlim(west, east)
    axis.set_ylim(south, north)


def figureFor(bounds, plt):
    west, south, east, north = bounds
    aspect = (north - south) / (east - west)
    width = 10.0
    return plt.subplots(figsize=(width, max(4.0, width * aspect * 0.92)), dpi=130)


def shadedRelief(path, limits=None):
    """Colour by height with hill shading; no data in pale blue. Limits [m NAP] default to the raster's own range."""
    from matplotlib.colors import LightSource, Normalize
    with rasterio.open(path) as source:
        data = source.read(1, masked=True).astype(np.float64)
        bounds = tuple(source.bounds)
        cell = abs(source.transform.a)
    valid = data.compressed()
    if valid.size == 0:
        image = np.ones(data.shape + (3,)) * NODATA_COLOUR
        return image, bounds, (0.0, 1.0)
    low, high = limits if limits else np.percentile(valid, [0.5, 99.5])
    high = max(high, low + 1.0)
    filled = data.filled(float(np.median(valid)))
    light = LightSource(azdeg=315, altdeg=40)
    image = light.shade(filled, cmap=colormap("terrain"), norm=Normalize(low, high), blend_mode="soft",
                        vert_exag=2.0, dx=cell, dy=cell)[..., :3]
    image[np.ma.getmaskarray(data)] = NODATA_COLOUR
    return image, bounds, (low, high)


def colourBar(figure, axis, limits, label, cmapName):
    from matplotlib import cm
    from matplotlib.colors import Normalize
    mappable = cm.ScalarMappable(norm=Normalize(*limits), cmap=colormap(cmapName))
    bar = figure.colorbar(mappable, ax=axis, fraction=0.035, pad=0.02)
    bar.set_label(label)


def previewRaster(path, record, overlays, outDir, plt):
    image, bounds, limits = shadedRelief(path)
    figure, axis = figureFor(bounds, plt)
    axis.imshow(image, extent=(bounds[0], bounds[2], bounds[1], bounds[3]), origin="upper", interpolation="nearest")
    drawOverlays(axis, overlays, bounds, showHouses=record["area"] == "park")
    kilometres(axis)
    colourBar(figure, axis, limits, "m NAP", "terrain")
    raster = record["raster"]
    axis.set_title("%s: %s, %g m cells, %d x %d, %.1f%% no data (pale blue)\nheights %s to %s m NAP, fetched %s"
                   % (record["name"], record["coverage"], record["cell_m"], raster["width"], raster["height"],
                      100.0 * raster["nodata_fraction"], raster["min"], raster["max"], record["downloaded_utc"][:10]),
                   fontsize=9)
    out = os.path.join(outDir, record["name"] + ".png")
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


def previewObjects(dtmPath, dsmPath, area, overlays, outDir, plt):
    """DSM minus DTM: houses and trees stand out; the survey's houses should sit on their blocks"""
    with rasterio.open(dtmPath) as dtmSource, rasterio.open(dsmPath) as dsmSource:
        if dtmSource.shape != dsmSource.shape or dtmSource.transform != dsmSource.transform:
            return None
        dtm = dtmSource.read(1, masked=True).astype(np.float64)
        dsm = dsmSource.read(1, masked=True).astype(np.float64)
        bounds = tuple(dtmSource.bounds)
    height = np.ma.masked_array(dsm.filled(np.nan) - dtm.filled(np.nan),
                                mask=np.ma.getmaskarray(dsm) | np.ma.getmaskarray(dtm))
    # Under buildings the DTM has no data: there the DSM stands above the ground around it
    figure, axis = figureFor(bounds, plt)
    shown = colormap("YlGn")(np.clip(height.filled(0.0), 0.0, 20.0) / 20.0)[..., :3]
    buildings = np.ma.getmaskarray(dtm) & ~np.ma.getmaskarray(dsm)
    shown[buildings] = (0.55, 0.35, 0.65)
    shown[np.ma.getmaskarray(dsm)] = NODATA_COLOUR
    axis.imshow(shown, extent=(bounds[0], bounds[2], bounds[1], bounds[3]), origin="upper", interpolation="nearest")
    drawOverlays(axis, overlays, bounds, showHouses=True)
    kilometres(axis)
    colourBar(figure, axis, (0.0, 20.0), "DSM minus DTM [m]", "YlGn")
    axis.set_title("%s: height of objects above the ground. Purple: no ground under it (buildings).\n"
                   "Black outlines: houses of the survey; they should sit on the purple blocks." % area, fontsize=9)
    out = os.path.join(outDir, "%s_objects.png" % area)
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


def previewOverview(records, rawDir, overlays, outDir, plt):
    """All areas on the coarsest raster at hand, the others on top in their own place"""
    def area(record):
        west, south, east, north = record["bounds_rd"]
        return (east - west) * (north - south)

    order = sorted(records, key=area, reverse=True)
    dtms = [r for r in order if r["coverage"] == "dtm_05m"] or order
    bounds = tuple(AREAS["horizon"]["bounds"])
    figure, axis = figureFor(bounds, plt)
    axis.set_facecolor(NODATA_COLOUR)
    limits = None   # one colour scale for all: that of the largest raster
    for record in dtms:
        image, rasterBounds, used = shadedRelief(os.path.join(rawDir, record["file"]), limits)
        limits = limits or used
        axis.imshow(image, extent=(rasterBounds[0], rasterBounds[2], rasterBounds[1], rasterBounds[3]),
                    origin="upper", interpolation="nearest")
    drawOverlays(axis, overlays, bounds, showHouses=False)
    kilometres(axis)
    if limits:
        colourBar(figure, axis, limits, "m NAP", "terrain")
    axis.set_title("Overview: %s\nRectangles: the areas fetched. Orange dashes: the current height model."
                   % ", ".join(r["name"] for r in dtms), fontsize=9)
    out = os.path.join(outDir, "overview.png")
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


def previewPlan(overlays, outDir, plt):
    """The planned areas before anything is fetched, on the current height map of the tip if there is one"""
    bounds = tuple(AREAS["horizon"]["bounds"])
    figure, axis = figureFor(bounds, plt)
    axis.set_facecolor(NODATA_COLOUR)
    background = "no background map found"
    if os.path.exists(OVERLAY_HEIGHTMAP) and os.path.exists(OVERLAY_MAP):
        registration = json.load(open(OVERLAY_HEIGHTMAP))
        anchor = json.load(open(OVERLAY_MAP))["metadata"]["anchor"]["rd"]
        imagePath = os.path.join(os.path.dirname(OVERLAY_HEIGHTMAP), registration["image"])
        if os.path.exists(imagePath):
            image = plt.imread(imagePath)
            metres = registration["metres_per_pixel"]
            column, row = registration["anchor_pixel"]
            west = anchor[0] - column * metres
            north = anchor[1] + row * metres
            east = west + image.shape[1] * metres
            south = north - image.shape[0] * metres
            axis.imshow(image, extent=(west, east, south, north), origin="upper", interpolation="bilinear")
            background = ("background: the current height map of the tip (%s), placed approximately"
                          % registration["image"])
    drawOverlays(axis, overlays, bounds, showHouses=False)
    kilometres(axis)
    lines = []
    for item in DOWNLOADS:
        width, height = gridSize(item)
        lines.append("%s %d x %d (%.0f MB)" % (item["name"], width, height, width * height * 4 / 1e6))
    axis.set_title("Planned areas, nothing fetched yet; %s\n%s" % (background, ", ".join(lines)), fontsize=8)
    os.makedirs(outDir, exist_ok=True)
    out = os.path.join(outDir, "plan.png")
    figure.savefig(out, bbox_inches="tight")
    plt.close(figure)
    return out


def drawPreviews(rawDir, outDir):
    plt = pyplot()
    os.makedirs(outDir, exist_ok=True)
    overlays = loadOverlays()
    records = []
    if os.path.isdir(rawDir):
        for name in sorted(os.listdir(rawDir)):
            if name.endswith(".tif.json"):
                records.append(json.load(open(os.path.join(rawDir, name))))
    if not records:
        print("No rasters in %s yet" % rawDir)
        return []
    written = [previewRaster(os.path.join(rawDir, r["file"]), r, overlays, outDir, plt) for r in records]
    for area in AREAS:
        dtm = [r for r in records if r["area"] == area and r["coverage"] == "dtm_05m"]
        dsm = [r for r in records if r["area"] == area and r["coverage"] == "dsm_05m"]
        if dtm and dsm:
            out = previewObjects(os.path.join(rawDir, dtm[0]["file"]), os.path.join(rawDir, dsm[0]["file"]),
                                 area, overlays, outDir, plt)
            if out:
                written.append(out)
    written.append(previewOverview(records, rawDir, overlays, outDir, plt))
    return written


# ---------------------------------------------------------------- main

def main():
    parser = argparse.ArgumentParser(description="Fetch the AHN height models from PDOK into raw/ahn/, once.")
    parser.add_argument("--dry-run", action="store_true", help="show the requests and draw the plan; fetch nothing")
    parser.add_argument("--preview-only", action="store_true", help="only draw the previews of what is in raw/ahn/")
    parser.add_argument("--verify", action="store_true", help="check the raw files against their checksums")
    parser.add_argument("--only", nargs="+", metavar="NAME", help="fetch only these (names from the list below)")
    parser.add_argument("--service", default=SERVICE_URL, help="WCS address (default: %(default)s)")
    parser.add_argument("--raw-dir", default=RAW_DIR)
    parser.add_argument("--preview-dir", default=PREVIEW_DIR)
    parser.add_argument("--no-preview", action="store_true")
    arguments = parser.parse_args()

    if arguments.verify:
        return 0 if verify(arguments.raw_dir) else 1
    if arguments.preview_only:
        for path in drawPreviews(arguments.raw_dir, arguments.preview_dir):
            print("  preview %s" % path)
        return 0

    chosen = DOWNLOADS
    if arguments.only:
        names = {item["name"] for item in DOWNLOADS}
        unknown = sorted(set(arguments.only) - names)
        if unknown:
            parser.error("unknown names %s; choose from %s" % (", ".join(unknown), ", ".join(sorted(names))))
        chosen = [item for item in DOWNLOADS if item["name"] in arguments.only]

    print("Areas (RD New, west south east north):")
    for name, area in AREAS.items():
        w, s, e, n = area["bounds"]
        print("  %-8s %d %d %d %d  (%.1f x %.1f km)" % (name, w, s, e, n, (e - w) / 1000.0, (n - s) / 1000.0))
    print("Files:")
    for item in chosen:
        width, height = gridSize(item)
        print("  %-16s %s, %g m cells, %d x %d, about %.0f MB" % (item["name"], item["coverage"], item["cell"],
                                                               width, height, width * height * 4 / 1e6))

    if arguments.dry_run:
        for item in chosen:
            print("  %s\n    %s" % (item["name"], coverageUrl(item, "<GeoTIFF format from DescribeCoverage>",
                                                            None, arguments.service)))
        if not arguments.no_preview:
            print("  preview %s" % previewPlan(loadOverlays(), arguments.preview_dir, pyplot()))
        return 0

    def complete(item):
        path = os.path.join(arguments.raw_dir, item["name"] + ".tif")
        return os.path.exists(path) and os.path.exists(path + ".json")

    missing = [item for item in chosen if not complete(item)]
    if missing:
        today = datetime.date.today().isoformat()
        print("Reading the service description from %s" % arguments.service)
        described = readServiceDescription(arguments.service, sorted({item["coverage"] for item in missing}),
                                           os.path.join(arguments.raw_dir, "service"), today, dryRun=False)
        rejectDir = os.path.join(arguments.preview_dir, "rejected")
        for item in chosen:
            download(item, described, arguments.service, arguments.raw_dir, rejectDir)
    else:
        print("Everything asked for is already in %s" % arguments.raw_dir)

    if not arguments.no_preview:
        for path in drawPreviews(arguments.raw_dir, arguments.preview_dir):
            print("  preview %s" % path)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, ValueError) as error:
        print("\nStopped: %s" % error, file=sys.stderr)
        sys.exit(1)
