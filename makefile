# Compiler and LEA settings (aligned with ed25519/makefile.lea)
STDLEA_CFLAGS   = -I/usr/local/include/stdlea
STDLEA_LDFLAGS  = -L/usr/local/lib -lstdlea

CFLAGS_WASM_BASE    := --target=wasm32 -ffreestanding -nostdlib -Wl,--no-entry -Os -Wall -Wextra -pedantic
CFLAGS_WASM_FEATURES:= -mbulk-memory -msign-ext -mmultivalue
CFLAGS_WASM         = $(CFLAGS_WASM_BASE) $(CFLAGS_WASM_FEATURES) $(STDLEA_CFLAGS) -flto

# Source files
SRC_ENC := encoder.c
SRC_DEC := decoder.c
ALL_SRCS_FOR_FORMAT := encoder.c decoder.c sctp.h

# Targets
TARGET_ENC := sctp.enc.wasm
TARGET_DEC := sctp.dec.wasm

.PHONY: all clean format check-unicode

all: $(TARGET_ENC) $(TARGET_DEC)

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

# Clean rule
clean:
	@echo "Removing build artifacts..."
	rm -f $(TARGET_ENC) $(TARGET_DEC) *.o

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