# SCTP API Reference

This document outlines the function layout for the SCTP encoder and decoder. The API operates on a global internal state, meaning you do not need to manage instance handles.

## Data Types

The API uses the following core structures and types:

```c
// Enum defining the 14 data types and EOF marker.
typedef enum {
    SCTP_TYPE_INT8, SCTP_TYPE_UINT8,
    SCTP_TYPE_INT16, SCTP_TYPE_UINT16,
    SCTP_TYPE_INT32, SCTP_TYPE_UINT32,
    SCTP_TYPE_INT64, SCTP_TYPE_UINT64,
    SCTP_TYPE_ULEB128, SCTP_TYPE_SLEB128,
    SCTP_TYPE_FLOAT32, SCTP_TYPE_FLOAT64,
    SCTP_TYPE_SHORT,
    SCTP_TYPE_VECTOR,
    SCTP_TYPE_EOF
} sctp_type_t;
```

---

## Decoder API (Callback-Based)

The decoder operates on a global state. Calling `sctp_decoder_init` resets this state for a new decoding session.

### Callback Handler

The host environment must provide a function that matches this signature. The decoder will call it for every field it parses.

```c
/**
 * @brief The user-defined callback function to handle decoded data.
 * @param type The data type of the field found.
 * @param data A pointer to the decoded data in the WASM memory.
 * @param size The size of the data in bytes.
 */
LEA_IMPORT(env, __sctp_data_handler)
void __sctp_data_handler(sctp_type_t type, const void* data, size_t size);
```

### Functions

```c
/**
 * @brief Initializes the decoder and allocates a buffer for the encoded data.
 *
 * This resets the decoder state and allocates a new buffer.
 *
 * @param size The size of the data buffer to allocate.
 * @return A pointer to the start of the writable data buffer.
 */
void* sctp_decoder_init(size_t size);

/**
 * @brief Runs the decoder over the buffer.
 * @return 0 on success or if EOF is reached, non-zero on error.
 */
int sctp_decoder_run(void);
```

### General Workflow (Decoder)

1.  Get a buffer from the decoder: `void* buffer = sctp_decoder_init(data_size);`
2.  Write your encoded data into that `buffer`.
3.  Start parsing: `sctp_decoder_run();`
4.  To decode new data, simply call `sctp_decoder_init` again.

---

## Encoder API

The encoder operates on a global state. Calling `sctp_encoder_init` resets this state for a new encoding session.

### Functions

```c
/**
 * @brief Initializes the encoder with a given buffer capacity.
 *
 * This resets the encoder state and allocates a new buffer.
 *
 * @param capacity The capacity of the internal buffer to allocate.
 */
void sctp_encoder_init(size_t capacity);

// Gets a read-only pointer to the encoded data buffer.
const uint8_t* sctp_encoder_data(void);

// Gets the current size of the encoded data in bytes.
size_t sctp_encoder_size(void);

// --- Add Functions (operate on the global encoder) ---

// Adds a vector and returns a pointer to the destination buffer for writing.
void* sctp_encoder_add_vector(size_t length);
void sctp_encoder_add_short(uint8_t value);
void sctp_encoder_add_int8(int8_t value);
void sctp_encoder_add_uint8(uint8_t value);
void sctp_encoder_add_int16(int16_t value);
void sctp_encoder_add_uint16(uint16_t value);
void sctp_encoder_add_int32(int32_t value);
void sctp_encoder_add_uint32(uint32_t value);
void sctp_encoder_add_int64(int64_t value);
void sctp_encoder_add_uint64(uint64_t value);
void sctp_encoder_add_uleb128(uint64_t value);
void sctp_encoder_add_sleb128(int64_t value);
void sctp_encoder_add_float32(float value);
void sctp_encoder_add_float64(double value);
void sctp_encoder_add_eof(void);
```

### General Workflow (Encoder)

1.  Initialize the encoder: `sctp_encoder_init(1024);`
2.  Add data: `sctp_encoder_add_int32(123);`
3.  Add a vector: `void* vec_ptr = sctp_encoder_add_vector(5);` Then, the host writes 5 bytes to `vec_ptr`.
4.  Finalize: `sctp_encoder_add_eof();`
5.  Get the result: `const uint8_t* data = sctp_encoder_data();`, `size_t size = sctp_encoder_size();`
6.  To start a new encoding, simply call `sctp_encoder_init` again.
