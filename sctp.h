#ifndef SCTP_H
#define SCTP_H

#include <stdlea.h>

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
 * @brief Opaque pointer to the decoder's state. Managed by the library.
 */
typedef struct sctp_decoder sctp_decoder_t;

/**
 * @brief Defines the 14 SCTP data types plus a reserved and EOF marker.
 *
 * These values correspond to the lower 4 bits of a field's header byte.
 */
typedef enum {
    SCTP_TYPE_INT8      = 0,
    SCTP_TYPE_UINT8     = 1,
    SCTP_TYPE_INT16     = 2,
    SCTP_TYPE_UINT16    = 3,
    SCTP_TYPE_INT32     = 4,
    SCTP_TYPE_UINT32    = 5,
    SCTP_TYPE_INT64     = 6,
    SCTP_TYPE_UINT64    = 7,
    SCTP_TYPE_ULEB128   = 8,
    SCTP_TYPE_SLEB128   = 9,
    SCTP_TYPE_FLOAT32   = 10,
    SCTP_TYPE_FLOAT64   = 11,
    SCTP_TYPE_SHORT     = 12,
    SCTP_TYPE_VECTOR    = 13,
    // Value 14 is reserved
    SCTP_TYPE_EOF       = 15
} sctp_type_t;


// --- Decoder API (Callback-Based) ---

/**
 * @brief The user-defined callback function to handle decoded data.
 *
 * This function is invoked by `sctp_decoder_run` for each data field found
 * in the stream.
 *
 * @param type The data type of the field found (e.g., SCTP_TYPE_INT32).
 * @param data A pointer to the decoded data within the WASM linear memory.
 * @param size The size of the data in bytes.
 * @param user_context A pointer to optional user-defined state, passed through
 *                     from `sctp_decoder_run`.
 */
typedef void (*sctp_data_handler_t)(
    sctp_type_t type,
    const void* data,
    size_t size,
    void* user_context
);

/**
 * @brief Allocates and initializes a new decoder context.
 * @param data A pointer to the byte buffer to be decoded.
 * @param size The size of the data buffer.
 * @return A pointer to the new decoder context.
 */
sctp_decoder_t* sctp_decoder_init(const void* data, size_t size);

/**
 * @brief Resets an existing decoder to parse a new data buffer.
 * @param dec A pointer to the decoder context to reset.
 * @param data A pointer to the new byte buffer to be decoded.
 * @param size The size of the new data buffer.
 */
void sctp_decoder_reset(sctp_decoder_t* dec, const void* data, size_t size);

/**
 * @brief Runs the decoder over its buffer, invoking a callback for each field.
 *
 * The callback handler is imported from the host environment.
 *
 * @param dec A pointer to the decoder context.
 * @param user_context An optional pointer to user state, passed to the handler.
 * @return 0 on success or if EOF is reached, a non-zero error code on failure.
 */
int sctp_decoder_run(sctp_decoder_t* dec, void* user_context);


// --- Encoder API ---

/**
 * @brief Allocates and initializes a new encoder context.
 * @param capacity The initial capacity of the internal buffer to allocate.
 * @return A pointer to the new encoder context.
 */
sctp_encoder_t* sctp_encoder_init(size_t capacity);

/**
 * @brief Resets an existing encoder to be reused, clearing its contents.
 * @param enc A pointer to the encoder context to reset.
 */
void sctp_encoder_reset(sctp_encoder_t* enc);

/**
 * @brief Gets a read-only pointer to the encoded data buffer.
 * @param enc A pointer to the encoder context.
 * @return A const pointer to the start of the encoded data.
 */
const uint8_t* sctp_encoder_data(const sctp_encoder_t* enc);

/**
 * @brief Gets the current size of the encoded data in bytes.
 * @param enc A pointer to the encoder context.
 * @return The number of bytes currently written to the buffer.
 */
size_t sctp_encoder_size(const sctp_encoder_t* enc);

// --- Encoder 'add' functions ---

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

#endif // SCTP_H
