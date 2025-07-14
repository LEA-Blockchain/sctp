# Compiler and LEA settings (aligned with ed25519/makefile.lea)
STDLEA_CFLAGS   = -I/usr/local/include/stdlea
STDLEA_LDFLAGS  = -L/usr/local/lib -lstdlea

CFLAGS_WASM_BASE    := --target=wasm32 -ffreestanding -nostdlib -Wl,--no-entry -Os -Wall -Wextra -pedantic
CFLAGS_WASM_FEATURES:= -mbulk-memory -msign-ext -mmultivalue
CFLAGS_WASM         = $(CFLAGS_WASM_BASE) $(CFLAGS_WASM_FEATURES) $(STDLEA_CFLAGS) -flto
CFLAGS_WASM_TEST    = $(CFLAGS_WASM) -DSCTP_HANDLER_PROVIDED -DENABLE_LEA_LOG

# Source files
SRC_ENC := encoder.c
SRC_DEC := decoder.c
SRC_TEST := test.c encoder.c decoder.c
ALL_SRCS_FOR_FORMAT := encoder.c decoder.c sctp.h test.c

# Targets
TARGET_ENC := sctp.enc.wasm
TARGET_DEC := sctp.dec.wasm
TARGET_TEST := test.wasm

.PHONY: all clean format check-unicode test

all: $(TARGET_ENC) $(TARGET_DEC) test

test: $(TARGET_TEST)
	@echo "\n--- Running Integration Test ---"
	@node test.js $(TARGET_TEST)
	@echo "--------------------------------\n"

# Rule for the encoder
$(TARGET_ENC): $(SRC_ENC)
	@echo "Compiling and linking $< to $@"
	clang $(CFLAGS_WASM) $< $(STDLEA_LDFLAGS) -o $@
	@echo "Stripping custom sections..."
	wasm-strip $@
	@echo "Dumping section and export info:"
	wasm-objdump -x $@
	@echo "Build complete: $@"

# Rule for the decoder
$(TARGET_DEC): $(SRC_DEC)
	@echo "Compiling and linking $< to $@"
	clang $(CFLAGS_WASM) $< $(STDLEA_LDFLAGS) -o $@
	@echo "Stripping custom sections..."
	wasm-strip $@
	@echo "Dumping section and export info:"
	wasm-objdump -x $@
	@echo "Build complete: $@"

# Rule for the test module
$(TARGET_TEST): $(SRC_TEST) sctp.h
	@echo "Compiling and linking test module to $@"
	clang $(CFLAGS_WASM_TEST) $(SRC_TEST) $(STDLEA_LDFLAGS) -o $@
	@echo "Build complete: $@"

# Clean rule
clean:
	@echo "Removing build artifacts..."
	rm -f $(TARGET_ENC) $(TARGET_DEC) $(TARGET_TEST) *.o

# Quality checks
format: check-unicode
	@echo "Formatting source files..."
	@clang-format -i $(ALL_SRCS_FOR_FORMAT)

# Quality checks
format: check-unicode
	@echo "Formatting source files..."
	@clang-format -i $(ALL_SRCS_FOR_FORMAT)

check-unicode:
	@echo "Checking for non-ASCII characters..."
	@if grep --color=auto -P -n "[^\x00-\x7F]" $(ALL_SRCS_FOR_FORMAT) > /dev/null; then \
		echo "❌ Unicode characters detected in source files!"; \
		grep --color=always -P -n "[^\x00-\x7F]" $(ALL_SRCS_FOR_FORMAT); \
		exit 1; \
	fi