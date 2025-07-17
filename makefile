# --- Configuration ---
ENABLE_UBSEN := 0
ENABLE_LEA_LOG := 0
ENABLE_LEA_FMT := 0

# --- Includes ---
include ../stdlea/stdlea.mk

# --- Compiler Flags ---
CFLAGS += -O3

# --- Include Paths ---
INCLUDE_PATHS := -I.

# Source files
ENC_SRCS := encoder.c
DEC_SRCS := decoder.c
TEST_SRCS := test.c encoder.c decoder.c
HDRS := sctp.h
SCTP_LOCAL_SRCS := $(ENC_SRCS) $(DEC_SRCS) $(TEST_SRCS)

# Targets
TARGET_ENC := sctp.enc.wasm
TARGET_DEC := sctp.dec.wasm
TARGET_TEST := test.wasm

.PHONY: all clean format check-unicode

all: $(TARGET_ENC) $(TARGET_DEC)

$(TARGET_ENC): $(ENC_SRCS) $(HDRS)
	@echo "Compiling and linking sources to $(TARGET_ENC)..."
	@echo "ENC_SRCS: $(ENC_SRCS)"
	@echo "SRCS: $(SRCS)"
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(ENC_SRCS) $(SRCS) -o $(TARGET_ENC)
	@echo "Stripping custom sections..."
	wasm-strip $(TARGET_ENC)

$(TARGET_DEC): $(DEC_SRCS) $(HDRS)
	@echo "Compiling and linking sources to $(TARGET_DEC)..."
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(DEC_SRCS) $(SRCS) -o $(TARGET_DEC)
	@echo "Stripping custom sections..."
	wasm-strip $(TARGET_DEC)

$(TARGET_TEST): CFLAGS += -DSCTP_HANDLER_PROVIDED
$(TARGET_TEST): $(TEST_SRCS) $(HDRS)
	@echo "Compiling and linking test module to $@"
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(TEST_SRCS) $(SRCS) -o $@
	@echo "Build complete: $@"

clean:
	@echo "Removing build artifacts..."
	rm -f $(TARGET_ENC) $(TARGET_DEC) $(TARGET_TEST) *.o

format: check-unicode
	@echo "Formatting source files..."
	@clang-format -i $(SCTP_LOCAL_SRCS) $(HDRS)

check-unicode:
	@echo "Checking for non-ASCII characters..."
	@if grep --color=auto -P -n "[^\x00-\x7F]" $(SCTP_LOCAL_SRCS) $(HDRS) > /dev/null; then
		echo "[FAIL] Unicode characters detected in source files!";
		grep --color=always -P -n "[^\x00-\x7F]" $(SCTP_LOCAL_SRCS) $(HDRS);
		exit 1;
	fi
