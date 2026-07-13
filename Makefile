CXX ?= g++
AR ?= ar
.DEFAULT_GOAL := all

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BUILD_DIR)/forevervalidator
COMMON_WARNINGS := -Wall -Wextra -Werror -Wno-deprecated-declarations
COMMON_CXXFLAGS := -O2 -ffp-contract=off -MMD -MP -std=c++17 $(COMMON_WARNINGS)
PRIVATE_CXXFLAGS := $(COMMON_CXXFLAGS) -Iinclude -Isrc
PUBLIC_CXXFLAGS := $(COMMON_CXXFLAGS) -Iinclude
LDLIBS := -lcrypto -lz -lm

CORE_SOURCES := $(shell find \
	src/engine \
	src/format \
	src/simulation \
	src/validation/api \
	src/validation/planning \
	src/validation/evaluation \
	-type f -name '*.cpp' -print | sort)
JSON_SOURCES := $(shell find src/validation/serialization -type f -name '*.cpp' -print | sort)
NATIVE_SOURCES := $(shell find src/platform/native -type f -name '*.cpp' -print | sort)
CLI_SOURCES := $(shell find src/app/cli -type f -name '*.cpp' -print | sort)

objects_for = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(1))
CORE_OBJECTS := $(call objects_for,$(CORE_SOURCES))
JSON_OBJECTS := $(call objects_for,$(JSON_SOURCES))
NATIVE_OBJECTS := $(call objects_for,$(NATIVE_SOURCES))
CLI_OBJECTS := $(call objects_for,$(CLI_SOURCES))

CORE_LIB := $(BUILD_DIR)/libforevervalidator_core.a
JSON_LIB := $(BUILD_DIR)/libforevervalidator_json.a
NATIVE_LIB := $(BUILD_DIR)/libforevervalidator_native.a
LIBRARIES := $(CORE_LIB) $(JSON_LIB) $(NATIVE_LIB)

.PHONY: all libraries cli-link clean

all: libraries cli-link

libraries: $(LIBRARIES)

cli-link: $(TARGET)

$(TARGET): $(CLI_OBJECTS) $(NATIVE_LIB) $(JSON_LIB) $(CORE_LIB) | $(BUILD_DIR)
	$(CXX) -o $@ $(CLI_OBJECTS) $(NATIVE_LIB) $(JSON_LIB) $(CORE_LIB) $(LDLIBS)

$(CORE_LIB): $(CORE_OBJECTS) | $(BUILD_DIR)
	rm -f $@
	$(AR) rcs $@ $^

$(JSON_LIB): $(JSON_OBJECTS) | $(BUILD_DIR)
	rm -f $@
	$(AR) rcs $@ $^

$(NATIVE_LIB): $(NATIVE_OBJECTS) | $(BUILD_DIR)
	rm -f $@
	$(AR) rcs $@ $^

$(OBJ_DIR)/src/app/cli/%.o: src/app/cli/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PUBLIC_CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(PRIVATE_CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

ALL_OBJECTS := $(CORE_OBJECTS) $(JSON_OBJECTS) $(NATIVE_OBJECTS) $(CLI_OBJECTS)
-include $(ALL_OBJECTS:.o=.d)
