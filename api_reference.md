# SCTP API Reference

This document outlines the function layout for the SCTP encoder and decoder.

## Data Types

The API uses the following core structures and types:

```c
// Opaque pointer to the encoder's state. Managed by the library.
typedef struct sctp_encoder sctp_encoder_t;

// Opaque pointer to the decoder's state. Managed by the library.
typedef struct sctp_decoder sctp_decoder_t;

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

### Callback Handler

The VM must provide a function that matches this signature. The decoder will call it for every field it parses.

```c
/**
 * @brief The user-defined callback function to handle decoded data.
 * @param type The data type of the field found.
 * @param data A pointer to the decoded data in the WASM memory.
 * @param size The size of the data in bytes.
 * @param user_context A pointer to optional user-defined state.
 */
typedef void (*sctp_data_handler_t)(
    sctp_type_t type,
    const void* data,
    size_t size,
    void* user_context
);
```

### Functions

```c
// Allocates and initializes a new decoder context for a given data buffer.
sctp_decoder_t* sctp_decoder_init(const void* data, size_t size);

// Resets an existing decoder to parse a new data buffer.
void sctp_decoder_reset(sctp_decoder_t* dec, const void* data, size_t size);

// Runs the decoder over the buffer, invoking the handler for each field.
// Returns 0 on success or if EOF is reached, non-zero on error.
int sctp_decoder_run(sctp_decoder_t* dec, sctp_data_handler_t handler, void* user_context);
```

### General Workflow (Decoder)

1.  Implement your `sctp_data_handler_t` callback function.
2.  Create a decoder instance: `sctp_decoder_t* dec = sctp_decoder_init(my_data, data_size);`
3.  Start parsing: `sctp_decoder_run(dec, my_callback, &my_state);`
4.  To reuse the decoder object with new data, call `sctp_decoder_reset(dec, new_data, new_size);`.

---

## Encoder API

### Functions

```c
// Allocates and initializes a new encoder context with a given buffer capacity.
sctp_encoder_t* sctp_encoder_init(size_t capacity);

// Resets an existing encoder to be reused, clearing its contents.
void sctp_encoder_reset(sctp_encoder_t* enc);

// Gets a read-only pointer to the encoded data buffer.
const uint8_t* sctp_encoder_data(const sctp_encoder_t* enc);

// Gets the current size of the encoded data in bytes.
size_t sctp_encoder_size(const sctp_encoder_t* enc);

// --- Add Functions ---

// Adds a vector and returns a pointer to the destination buffer for writing.
void* sctp_encoder_add_vector(sctp_encoder_t* enc, size_t length);
void sctp_encoder_add_short(sctp_encoder_t* enc, uint8_t value);
void sctp_encoder_add_int8(sctp_encoder_t* enc, int8_t value);
void sctp_encoder_add_uint8(sctp_encoder_t* enc, uint8_t value);
void sctp_encoder_add_int16(sctp_encoder_t* enc, int16_t value);
void sctp_encoder_add_uint16(sctp_encoder_t* enc, uint16_t value);
void sctp_encoder_add_int32(sctp_encoder_t* enc, int32_t value);
void sctp_encoder_add_uint32(sctp_encoder_t* enc, uint32_t value);
void sctp_encoder_add_int64(sctp_encoder_t* enc, int64_t value);
void sctp_encoder_add_uint64(sctp_encoder_t* enc, uint64_t value);
void sctp_encoder_add_uleb128(sctp_encoder_t* enc, uint64_t value);
void sctp_encoder_add_sleb128(sctp_encoder_t* enc, int64_t value);
void sctp_encoder_add_float32(sctp_encoder_t* enc, float value);
void sctp_encoder_add_float64(sctp_encoder_t* enc, double value);
void sctp_encoder_add_eof(sctp_encoder_t* enc);
```

### General Workflow (Encoder)

1.  Create an encoder: `sctp_encoder_t* enc = sctp_encoder_init(1024);`
2.  Add data: `sctp_encoder_add_int32(enc, 123);`
3.  Add a vector: `void* vec_ptr = sctp_encoder_add_vector(enc, 5);` Then, the host writes 5 bytes to `vec_ptr`.
4.  Finalize: `sctp_encoder_add_eof(enc);`
5.  Get the result: `const uint8_t* data = sctp_encoder_data(enc);`, `size_t size = sctp_encoder_size(enc);`
6.  To reuse the encoder, call `sctp_encoder_reset(enc);`.
