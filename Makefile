# Slufter3D - build file
#   make test     native unit tests of the models (any C++17 compiler), and the checks of data/
#   make houses   regenerate data/houses.json from raw/ (needs Python, see tools/make_houses.py)

CXX ?= c++

SOURCES := core/Mesh.cpp $(wildcard models/*.cpp)
HEADERS := $(wildcard core/*.h) $(wildcard models/*.h)

.PHONY: test houses clean

build/test_models: $(SOURCES) tests/test_models.cpp $(HEADERS)
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Icore -Imodels $(SOURCES) tests/test_models.cpp -o $@

test: build/test_models
	./build/test_models
	python3 tools/check_data.py

houses:
	python3 tools/make_houses.py

clean:
	rm -rf build
