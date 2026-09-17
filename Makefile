# --- Project Metadata ---
NAME     = json2dbcli
MAJOR    = 0
MINOR    = 1
RELEASE  = 0
VERSION  = $(MAJOR).$(MINOR).$(RELEASE)
BRIEF    = "sim760x enable and telemetry"

# --- Toolchain Discovery ---
CC      = clang
AR      = llvm-ar
STRIP   = llvm-strip
PKG_CON = pkg-config

# --- Arch Lookup ---
TRIPLE_x86_64  = x86_64-linux-gnu
TRIPLE_arm     = arm-linux-gnueabihf
TRIPLE_aarch64 = aarch64-linux-gnu

# Default choice
ARCH ?= x86_64
TARGET_TRIPLE := $(TRIPLE_$(ARCH))
ifeq ($(TARGET_TRIPLE),)
$(error Invalid ARCH '$(ARCH)'. Supported: x86_64, arm, aarch64)
endif

# --- Debug ---
# Disable by: make DEBUG=DISABLE
DEBUG ?= 

# --- Paths ---
SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build/$(TARGET_TRIPLE)
OUT_DIR   = release/$(TARGET_TRIPLE)
DOC_DIR   = docs

# --- Compiler Flags (GDB Friendly) ---
# -Og: Optimize for debugging experience
# -ggdb3: Include maximum GDB-specific debug info
# -fPIC: Needed for shared libraries
CFLAGS  = -std=gnu99 -fPIC -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wno-gnu-zero-variadic-macro-arguments
CFLAGS += -I$(INC_DIR) -Og -ggdb3 -D_GNU_SOURCE
CFLAGS += --target=$(TARGET_TRIPLE)
CFLAGS += -D$(DEBUG)_LOGGING
LDFLAGS = --target=$(TARGET_TRIPLE)
LIBS    = libpq

# --- Target Files ---
OBJ = $(BUILD_DIR)/$(NAME).o
EXE = $(BUILD_DIR)/$(NAME).exe

# --- Pkg-Config Flags ---
CFLAGS  += $(shell pkg-config --cflags $(LIBS))
LDLIBS   = $(shell pkg-config --libs $(LIBS))

# --- Compilation ---
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "  CC      $< [$(TARGET_TRIPLE)]"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Linking ---
$(EXE): $(OBJ)
	@echo "  LD      $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	
# --- Release/Cross-Compile Target ---
# Usage: make release ARCH=arm
release: clean all
	@mkdir -p $(OUT_DIR)
	@cp $(INC_DIR)/*.h $(OUT_DIR)/
	@cp $(LIB_A) $(LIB_SO) $(PC_FILE) $(OUT_DIR)/
	@cp -r $(DOC_DIR)/html $(OUT_DIR)/
	@cp -r $(DOC_DIR)/man $(OUT_DIR)/
	@zip -r $(OUT_DIR)_lib$(NAME).$(MAJOR).zip $(OUT_DIR) >> /dev/null
	@echo "  GEN     $(OUT_DIR)_lib$(NAME).$(MAJOR).zip"

# --- Documentation with doxygen ---
docs:
	@cp $(DOC_DIR)/Doxyfile $(DOC_DIR)/Doxyfile.tmp
	@sed -i 's|^PROJECT_NAME.*|PROJECT_NAME = $(NAME)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_NAME_BRIEF.*|PROJECT_NAME_BRIEF = $(BRIEF)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_BRIEF.*|PROJECT_BRIEF = $(BRIEF)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_NUMBER.*|PROJECT_NUMBER = $(VERSION)|' docs/Doxyfile.tmp
	@echo "  GEN     $(DOC_DIR)/html"
	@echo "  GEN     $(DOC_DIR)/xml"
	@doxygen $(DOC_DIR)/Doxyfile.tmp >> /dev/null
	@rm $(DOC_DIR)/Doxyfile.tmp
	@mkdir -p $(DOC_DIR)/man
	@echo "  GEN     $(DOC_DIR)/man"
	@doxy2man --novalidate --nowarn --nosummary --pkg "$(NAME) man page" $(DOC_DIR)/xml/xcserial_8h.xml -o $(DOC_DIR)/man

# --- Dynamic Test Suite ---
.PRECIOUS: $(BUILD_DIR)/test/%
$(BUILD_DIR)/test/%: test/%.c $(LIB_SO)
	@mkdir -p $(BUILD_DIR)/test
	@echo "  TEST    $< -> $@"
	@$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -l$(NAME) $(LIBS) -Wl,-rpath,$(abspath $(BUILD_DIR))
test-%: $(BUILD_DIR)/test/%
	@echo "  RUN     $<"
	@$<
	
# --- Cleanup ---
clean:
	rm -rf build/$(TARGET_TRIPLE) release/$(TARGET_TRIPLE) 
cleanall:
	rm -rf build/ release/ $(DOC_DIR)/html $(DOC_DIR)/man $(DOC_DIR)/xml

$(BUILD_DIR):
	mkdir -p $@

.PHONY: all clean release docs test-%