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


// --- Decoder API ---

/**
 * @brief Allocates memory for a decoder and its data buffer.
 * @param size The size of the data buffer to allocate.
 * @return A pointer to the start of the writable data buffer.
 */
void* sctp_decoder_init(size_t size);

/**
 * @brief Runs the decoder over the buffer.
 * @return 0 on success or if EOF is reached, non-zero on error.
 */
int sctp_decoder_run(void);


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
const uint8_t* sctp_encoder_data(void);

/**
 * @brief Gets the current size of the encoded data.
 * @return The number of bytes currently written to the buffer.
 */
size_t sctp_encoder_size(void);

// --- Encoder 'add' functions ---

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


#endif // SCTP_H
