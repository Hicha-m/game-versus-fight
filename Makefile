.PHONY: help configure build run test clean rebuild

help:
	@echo "Targets available:"
	@echo "  make configure  - cmake --preset dev"
	@echo "  make build      - cmake --build --preset build"
	@echo "  make run        - cmake --build --preset run"
	@echo "  make test       - ctest --preset test"
	@echo "  make clean      - remove build/"
	@echo "  make rebuild    - clean + configure + build"

configure:
	cmake --preset dev

build: configure
	cmake --build --preset build

run: configure
	cmake --build --preset run

test: build
	ctest --preset test

clean:
	cmake -E rm -rf build

rebuild: clean build
