# Slufter3D - build file
#   make test     native unit tests (any C++17 compiler) and the checks of data/
#   make wasm     docs/slufter.wasm (needs Emscripten, see README.md)
#   make smoke    a short visit through the built docs/slufter.wasm (needs Node)
#   make serve    local web server on http://localhost:8000
#   make houses   regenerate data/houses.json from raw/ (needs Python, see tools/make_houses.py)
#   make trees    regenerate data/trees.json from raw/ (needs Python, see tools/make_trees.py)
#   make world    regenerate the ground and docs/data/world.bin from raw/ and data/ (needs Python)

EMXX ?= em++
CXX  ?= c++

MODELS := core/Mesh.cpp $(wildcard models/*.cpp)
APP := src/WorldData.cpp src/Terrain.cpp src/Reception.cpp src/World.cpp src/Walker.cpp src/Scene.cpp src/Game.cpp
HEADERS := $(wildcard core/*.h) $(wildcard models/*.h) $(wildcard src/*.h)
INCLUDES := -Icore -Imodels -Isrc

WASM_FLAGS := -std=c++17 -O2 -Wall -Wextra -fno-exceptions -fno-rtti \
	-sSTANDALONE_WASM --no-entry -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728

.PHONY: all test wasm smoke serve houses trees world clean

all: test wasm

build/test_models: $(MODELS) tests/test_models.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra $(INCLUDES) $(MODELS) tests/test_models.cpp -o $@

build/test_world: $(MODELS) $(APP) tests/test_world.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra $(INCLUDES) $(MODELS) $(APP) tests/test_world.cpp -o $@

test: build/test_models build/test_world
	./build/test_models
	./build/test_world
	python3 tools/check_data.py

wasm: docs/slufter.wasm

docs/slufter.wasm: $(MODELS) $(APP) src/wasm_api.cpp $(HEADERS)
	$(EMXX) $(WASM_FLAGS) $(INCLUDES) $(MODELS) $(APP) src/wasm_api.cpp -o $@

smoke:
	node tests/smoke.mjs

serve:
	python3 -m http.server 8000 --directory docs

houses:
	python3 tools/make_houses.py

# THRESHOLD: lower than this above the ground is not a tree [m]
THRESHOLD ?= 1.0
trees:
	python3 tools/make_trees.py --threshold $(THRESHOLD)

world:
	python3 tools/make_terrain.py
	python3 tools/pack_world.py

clean:
	rm -rf build
