# SCTP-Core: Simple Compact Transaction Protocol for Lea

**A secure, minimal, and verifiable implementation of the Simple Compact Transaction Protocol (SCTP) standard, tailored for the Lea blockchain.**

This repository provides the core C implementation of SCTP, a highly efficient binary format for serializing transaction data. It is designed with security, performance, and auditability in mind, making it suitable for LEA blockchain applications.

## Key Features

*   **Efficient Binary Packing:** SCTP uses a combination of fixed-size types, LEB128 variable-length integers, and bit-packing to achieve a compact representation of data.
*   **Strongly Typed:** A clear set of 14 data types ensures data integrity and simplifies parsing.
*   **Callback-Driven Decoder:** The decoder uses a non-allocating, callback-based approach, making it flexible and memory-safe.
*   **WASM-Ready:** Designed from the ground up for WebAssembly, with a clean API for interacting with a host environment.
*   **Auditable Codebase:** The implementation is intentionally kept small and focused on the essential features required by the Lea blockchain, reducing the attack surface and simplifying security audits.

## Getting Started

### Prerequisites

*   A C compiler (e.g., GCC, Clang)
*   `make`

### Building

The `makefile` provides several targets to build the different WebAssembly modules.

*   **`make all`**: (Default) Builds all WebAssembly modules (`wasm_mvp` and `wasm_vm`).
*   **`make wasm_mvp`**: Builds the encoder and decoder for the standard WebAssembly MVP target.
*   **`make wasm_vm`**: Builds the encoder and decoder for the Lea VM target, which includes more advanced WASM features.

This will produce the following output files:
*   `sctp.mvp.enc.wasm`
*   `sctp.mvp.dec.wasm`
*   `sctp.vm.enc.wasm`
*   `sctp.vm.dec.wasm`

### Cleaning

To remove all build artifacts, run:
```bash
make clean
```

## Testing

This project includes a test suite that runs in a Node.js environment, exercising the WebAssembly module.

### Prerequisites

*   Node.js

### Running Tests

To run the tests, execute the following command:
```bash
node test.js
```
This command will build the WASM module and then execute the test suite defined in `test.js`.

## Fuzzing

The project also includes a fuzzer to help discover edge cases and potential vulnerabilities in the decoder.

### Running the Fuzzer

To run the fuzzer, execute the following command:
```bash
node fuzzer_runner.js
```

The fuzzer will continuously generate and test random data against the decoder and report any failures.

## API Usage

The library exposes a simple C API for encoding and decoding data. For detailed information, please see the [API Reference](./api_reference.md).

### Encoder Workflow

1.  **Initialize:** Create an encoder instance.
    ```c
    sctp_encoder_t* enc = sctp_encoder_init(1024); // Initial capacity of 1KB
    ```
2.  **Add Data:** Use the `sctp_encoder_add_*` functions to append data to the buffer.
    ```c
    sctp_encoder_add_int32(enc, 12345);
    sctp_encoder_add_uleb128(enc, 9876543210);
    ```
3.  **Finalize:** Add the End-of-File (EOF) marker.
    ```c
    sctp_encoder_add_eof(enc);
    ```
4.  **Retrieve Data:** Get a pointer to the encoded data and its size.
    ```c
    const uint8_t* data = sctp_encoder_data(enc);
    size_t size = sctp_encoder_size(enc);
    ```

### Decoder Workflow

1.  **Implement a Handler:** Create a callback function to process decoded fields.
    ```c
    void my_data_handler(sctp_type_t type, const void* data, size_t size, void* user_context) {
        // Process the data based on its type
    }
    ```
2.  **Initialize:** Create a decoder instance with the data to be parsed.
    ```c
    sctp_decoder_t* dec = sctp_decoder_init(encoded_data, data_size);
    ```
3.  **Run:** Start the decoding process.
    ```c
    int result = sctp_decoder_run(dec, my_data_handler, NULL);
    if (result != 0) {
        // Handle decoding error
    }
    ```

## Encoding Specification

For a complete definition of the binary format, please see the [SCTP Encoding Specification](./sctp_encoding.md).

## Contributing

This repository is primarily for maintaining the stable, core SCTP implementation for the Lea Blockchain. Contributions should be focused on critical bug fixes. New feature development should be directed to the main SCTP repository.

## License

This project is licensed under the [MIT License](./LICENSE).
