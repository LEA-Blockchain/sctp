#ifndef SCTP_H
#define SCTP_H

#include <stdint.h>
#include <stdlea.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @file sctp.h
 * @brief Public API for the Simple Compact Transaction Protocol (SCTP) library.
 *
 * This file defines the data structures, types, and functions for encoding
 * and decoding data using SCTP. The library is designed for WebAssembly
 * environments and uses a bump allocator model.
 */

// --- Core Data Types ---

/**
 * @brief Opaque pointer to the encoder's state. Managed by the library.
 */
typedef struct sctp_encoder sctp_encoder_t;

/**
 * @brief Defines the 14 SCTP data types plus a reserved and EOF marker.
 *
 * These values correspond to the lower 4 bits of a field's header byte.
 */
typedef enum
{
    SCTP_TYPE_INT8 = 0,
    SCTP_TYPE_UINT8 = 1,
    SCTP_TYPE_INT16 = 2,
    SCTP_TYPE_UINT16 = 3,
    SCTP_TYPE_INT32 = 4,
    SCTP_TYPE_UINT32 = 5,
    SCTP_TYPE_INT64 = 6,
    SCTP_TYPE_UINT64 = 7,
    SCTP_TYPE_ULEB128 = 8,
    SCTP_TYPE_SLEB128 = 9,
    SCTP_TYPE_FLOAT32 = 10,
    SCTP_TYPE_FLOAT64 = 11,
    SCTP_TYPE_SHORT = 12,
    SCTP_TYPE_VECTOR = 13,
    // Value 14 is reserved
    SCTP_TYPE_EOF = 15
} sctp_type_t;

/**
 * @brief A union holding the value of a decoded SCTP field.
 *
 * The field to access depends on the `sctp_type_t` of the decoded item.
 * For `SCTP_TYPE_VECTOR`, the `ptr` field points to the data.
 */
typedef union {
    int8_t as_int8;
    uint8_t as_uint8;
    int16_t as_int16;
    uint16_t as_uint16;
    int32_t as_int32;
    uint32_t as_uint32;
    int64_t as_int64;
    uint64_t as_uint64;
    uint64_t as_uleb128;
    int64_t as_sleb128;
    float as_float32;
    double as_float64;
    uint8_t as_short;
    const void* as_ptr;
} sctp_value_t;

/**
 * @brief Holds the state of the SCTP decoder.
 *
 * This structure tracks the data buffer being read, its total size, and the
 * current read position. It also stores the last decoded item for the
 * stateful API.
 */
typedef struct sctp_decoder {
    const uint8_t* data;     ///< Pointer to the input data buffer.
    size_t size;             ///< Total size of the data buffer in bytes.
    size_t position;         ///< Current read offset in the buffer.
    sctp_type_t last_type;   ///< Type of the last decoded item.
    sctp_value_t last_value; ///< Value of the last decoded item.
    size_t last_size;        ///< Size of the last decoded item.
    bool is_external_buffer; ///< True if the buffer is managed externally.
} sctp_decoder_t;


// --- Decoder API ---

/**
 * @brief Creates a new decoder instance and allocates a data buffer for it.
 *
 * The returned decoder's data buffer must be filled by the caller.
 * The instance must be freed with `sctp_decoder_free`.
 *
 * @param size The size of the data buffer to allocate.
 * @return A pointer to a new `sctp_decoder_t` instance, or NULL on failure.
 */
sctp_decoder_t* sctp_decoder_init(size_t size);

/**
 * @brief Creates a new decoder instance that reads from an existing buffer.
 *
 * This function does not copy the buffer; it only stores a pointer to it.
 * The instance must be freed with `sctp_decoder_free`.
 *
 * @param buffer Pointer to the data buffer to decode.
 * @param size The size of the data buffer.
 * @return A pointer to a new `sctp_decoder_t` instance, or NULL on failure.
 */
sctp_decoder_t* sctp_decoder_from_buffer(const void* buffer, size_t size);

/**
 * @brief Gets a pointer to the writable data buffer of a decoder instance.
 *
 * This is only valid for decoders created with `sctp_decoder_init`.
 *
 * @param dec The decoder instance.
 * @return A pointer to the start of the writable data buffer.
 */
void* sctp_decoder_get_buffer(sctp_decoder_t* dec);

/**
 * @brief Decodes the next field from the stream in a stateful manner.
 *
 * After calling this, you can get the decoded data type, value, and size
 * by directly accessing the members of the `sctp_decoder_t` struct.
 *
 * @param dec The decoder instance.
 * @return The `sctp_type_t` of the decoded field, or `SCTP_TYPE_EOF` if the
 *         stream has been fully read.
 */
sctp_type_t sctp_decoder_next(sctp_decoder_t* dec);

/**
 * @brief Runs the decoder over the buffer using a callback for each field.
 * @param dec The decoder instance.
 * @return 0 on success or if EOF is reached, non-zero on error.
 */
int sctp_decoder_run(sctp_decoder_t* dec);

// --- Encoder API ---

/**
 * @brief Initializes the encoder, resetting any previous state.
 * @param capacity The initial capacity of the internal buffer to allocate.
 */
void sctp_encoder_init(size_t capacity);

/**
 * @brief Gets a read-only pointer to the encoded data buffer.
 * @return A const pointer to the start of the encoded data.
 */
const uint8_t *sctp_encoder_data(void);

/**
 * @brief Gets the current size of the encoded data.
 * @return The number of bytes currently written to the buffer.
 */
size_t sctp_encoder_size(void);

// --- Encoder 'add' functions ---

/**
 * @brief Appends a variable-length data vector to the stream.
 * @param length The size of the vector in bytes.
 * @return A writable pointer to the allocated space in the buffer for the vector data.
 */
void *sctp_encoder_add_vector(size_t length);

/**
 * @brief Appends raw, unprocessed bytes to the stream.
 * @param data A pointer to the data to append.
 * @param length The size of the data in bytes.
 */
void sctp_encoder_add_raw(const void* data, size_t length);

/**
 * @brief Appends a short integer (0-15) to the stream.
 * @param value The 4-bit value to encode. Must be <= 15.
 */
void sctp_encoder_add_short(uint8_t value);

/**
 * @brief Appends an 8-bit signed integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_int8(int8_t value);

/**
 * @brief Appends an 8-bit unsigned integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_uint8(uint8_t value);

/**
 * @brief Appends a 16-bit signed integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_int16(int16_t value);

/**
 * @brief Appends a 16-bit unsigned integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_uint16(uint16_t value);

/**
 * @brief Appends a 32-bit signed integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_int32(int32_t value);

/**
 * @brief Appends a 32-bit unsigned integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_uint32(uint32_t value);

/**
 * @brief Appends a 64-bit signed integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_int64(int64_t value);

/**
 * @brief Appends a 64-bit unsigned integer to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_uint64(uint64_t value);

/**
 * @brief Appends a ULEB128-encoded 64-bit unsigned integer.
 * @param value The value to encode.
 */
void sctp_encoder_add_uleb128(uint64_t value);

/**
 * @brief Appends an SLEB128-encoded 64-bit signed integer.
 * @param value The value to encode.
 */
void sctp_encoder_add_sleb128(int64_t value);

/**
 * @brief Appends a 32-bit float to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_float32(float value);

/**
 * @brief Appends a 64-bit float (double) to the stream.
 * @param value The value to encode.
 */
void sctp_encoder_add_float64(double value);

/**
 * @brief Appends an End-Of-File marker to the stream.
 */
void sctp_encoder_add_eof(void);

#endif // SCTP_H
