# physengine — Makefile
# Сборка: ядро (статическая библиотека) + CLI + тесты.
# Зависимости: g++ >= 13 (C++20), make. Опционально ffmpeg для MP4.

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
INCLUDES  = -Iinclude -Ithird_party
BUILD    ?= build
BIN       ?= bin

SRC_CORE  = $(filter-out $(SRC_CLI),$(wildcard src/*.cpp))
SRC_CLI   = src/main.cpp
SRC_TESTS = $(wildcard tests/*.cpp)

LIB       = $(BUILD)/libphys.a
CLI       = $(BIN)/physengine
TESTS     = $(BUILD)/tests
OUT       ?= output
FPS       ?= 60

.PHONY: all clean test run check demo demo_video demo_clean fmt help

all: $(CLI)

# --- Ядро ---
$(BUILD):
	@mkdir -p $(BUILD) $(BIN)

$(BUILD)/%.o: src/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(LIB): $(SRC_CORE:src/%.cpp=$(BUILD)/%.o) | $(BUILD)
	ar rcs $@ $^

# --- CLI ---
$(CLI): $(SRC_CLI) $(LIB) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIB) -o $@

# --- Тесты (doctest, header-only) ---
$(TESTS): $(SRC_TESTS) $(LIB) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIB) -o $@

test: $(TESTS)
	./$(TESTS)

# Прогон сцены: make run SCENE=examples/oscillator.scene
run: $(CLI)
	./$(CLI) $(SCENE)

# Полная проверка CI: сборка + тесты + smoke
check: all test
	@echo "=== check OK ==="

# --- Визуализируемые демо ---
# Двойная звезда: PNG-кадры + CSV в $(OUT)
demo: $(CLI)
	$(CLI) demo --outdir $(OUT)

# Видео из кадров (требует ffmpeg)
demo_video: demo
	ffmpeg -y -loglevel error -framerate $(FPS) -i $(OUT)/frame_%05d.png \
	  -c:v libx264 -pix_fmt yuv420p -crf 20 $(OUT)/binary_stars.mp4
	@echo "видео: $(OUT)/binary_stars.mp4"

demo_clean:
	rm -rf $(OUT)

clean: demo_clean
	rm -rf $(BUILD) $(BIN)

help:
	@echo "Цели: all | test | run SCENE=... | check | demo | demo_video | clean"
