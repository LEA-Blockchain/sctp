#include "sctp.h"

/**
 * @file decoder.c
 * @brief Implements the Simple Compact Transaction Protocol (SCTP) decoder.
 *
 * This file contains the internal data structures and logic for parsing a
 * byte stream according to the SCTP format. It uses a callback-based approach
 * to notify the user of decoded data fields. It is designed for use in
 * WebAssembly environments.
 */

// --- Internal Constants ---

#define SCTP_TYPE_MASK 0x0F
#define SCTP_META_MASK 0xF0
#define SCTP_META_SHIFT 4
#define SCTP_VECTOR_LARGE_FLAG 0x0F

// --- Internal Struct Definition ---

/**
 * @brief Holds the state of the SCTP decoder.
 *
 * This structure tracks the data buffer being read, its total size, and the
 * current read position. It is not meant to be manipulated directly by the user.
 */
struct sctp_decoder {
    const uint8_t* data; ///< Pointer to the input data buffer.
    size_t size;         ///< Total size of the data buffer in bytes.
    size_t position;     ///< Current read offset in the buffer.
};

static sctp_decoder_t* g_decoder = NULL;

// --- LEB128 Decoding ---

/**
 * @brief Reads a single byte from the decoder's stream.
 * @param dec A pointer to the decoder context.
 * @param byte A pointer to a uint8_t to store the read byte.
 * @return 0 on success, -1 on end-of-file.
 */
static int _sctp_decoder_read_byte(sctp_decoder_t* dec, uint8_t* byte) {
    if (dec->position >= dec->size) {
        return -1; // EOF
    }
    *byte = dec->data[dec->position++];
    return 0;
}

/**
 * @brief Reads a block of raw data from the decoder's stream.
 *
 * If the requested read would go past the end of the buffer, this function
 * will trigger an abort.
 *
 * @param dec A pointer to the decoder context.
 * @param size The number of bytes to read.
 * @return A const pointer to the start of the read data within the stream.
 */
static const void* _sctp_decoder_read_data(sctp_decoder_t* dec, size_t size) {
    if (dec->position + size > dec->size) {
        lea_abort("unexpected end of stream while reading raw data");
        return NULL;
    }
    const void* ptr = dec->data + dec->position;
    dec->position += size;
    return ptr;
}

/**
 * @brief Decodes a 64-bit unsigned integer from the stream using ULEB128 format.
 * @param dec A pointer to the decoder context.
 * @return The decoded uint64_t value.
 * @note Aborts if the stream ends unexpectedly or if an overflow occurs.
 */
static uint64_t _sctp_decoder_read_uleb128(sctp_decoder_t* dec) {
    uint64_t result = 0;
    uint8_t shift = 0;
    uint8_t byte;
    while (1) {
        if (_sctp_decoder_read_byte(dec, &byte) != 0) {
            lea_abort("unexpected end of stream while reading uleb128");
        }
        result |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            break;
        }
        shift += 7;
        if (shift >= 64) {
            lea_abort("uleb128 overflow");
        }
    }
    return result;
}

/**
 * @brief Decodes a 64-bit signed integer from the stream using SLEB128 format.
 * @param dec A pointer to the decoder context.
 * @return The decoded int64_t value.
 * @note Aborts if the stream ends unexpectedly or if an overflow occurs.
 */
static int64_t _sctp_decoder_read_sleb128(sctp_decoder_t* dec) {
    int64_t result = 0;
    uint8_t shift = 0;
    uint8_t byte;
    while (1) {
        if (_sctp_decoder_read_byte(dec, &byte) != 0) {
            lea_abort("unexpected end of stream while reading sleb128");
        }
        result |= (int64_t)(byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) {
            if ((shift < 64) && (byte & 0x40)) {
                result |= -((int64_t)1 << shift);
            }
            break;
        }
        if (shift >= 64) {
            lea_abort("sleb128 overflow");
        }
    }
    return result;
}

/**
 * @brief Declaration of the imported host function for handling decoded data.
 * @see sctp_data_handler_t
 */
LEA_IMPORT(env, __sctp_data_handler)
void __sctp_data_handler(sctp_type_t type, const void* data, size_t size);

// --- Decoder Public API Implementation ---

LEA_EXPORT(sctp_decoder_init)
void* sctp_decoder_init(size_t size) {
    allocator_reset();
    sctp_decoder_t* dec = malloc(sizeof(sctp_decoder_t));
    if (!dec) lea_abort("malloc failed for sctp_decoder_t");

    dec->data = malloc(size);
    if (!dec->data) lea_abort("malloc failed for decoder buffer");
    
    dec->size = size;
    dec->position = 0;
    
    // The global pointer is now the handle
    g_decoder = dec;

    return (void*)dec->data;
}

LEA_EXPORT(sctp_decoder_run)
int sctp_decoder_run(void) {
    sctp_decoder_t* dec = g_decoder;
    if (!dec) lea_abort("decoder not initialized");
    
    while (dec->position < dec->size) {
        uint8_t header;
        if (_sctp_decoder_read_byte(dec, &header) != 0) {
            return 0; // Clean EOF
        }

        sctp_type_t type = (sctp_type_t)(header & SCTP_TYPE_MASK);
        uint8_t meta = (header & SCTP_META_MASK) >> SCTP_META_SHIFT;

        if (type == SCTP_TYPE_EOF) {
            __sctp_data_handler(type, NULL, 0);
            return 0;
        }

        const void* data;
        size_t size;

        switch (type) {
            case SCTP_TYPE_INT8:    size = 1; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_UINT8:   size = 1; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_INT16:   size = 2; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_UINT16:  size = 2; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_INT32:   size = 4; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_UINT32:  size = 4; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_INT64:   size = 8; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_UINT64:  size = 8; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_FLOAT32: size = 4; data = _sctp_decoder_read_data(dec, size); break;
            case SCTP_TYPE_FLOAT64: size = 8; data = _sctp_decoder_read_data(dec, size); break;
            
            case SCTP_TYPE_ULEB128: {
                uint64_t value = _sctp_decoder_read_uleb128(dec);
                __sctp_data_handler(type, &value, sizeof(value));
                continue; 
            }
            case SCTP_TYPE_SLEB128: {
                int64_t value = _sctp_decoder_read_sleb128(dec);
                __sctp_data_handler(type, &value, sizeof(value));
                continue;
            }
            case SCTP_TYPE_SHORT:
                data = &meta;
                size = 1;
                break;
            case SCTP_TYPE_VECTOR:
                if (meta == SCTP_VECTOR_LARGE_FLAG) {
                    size = (size_t)_sctp_decoder_read_uleb128(dec);
                } else {
                    size = meta;
                }
                data = _sctp_decoder_read_data(dec, size);
                break;

            default:
                lea_abort("unknown sctp type");
                return -1; // Error
        }
        __sctp_data_handler(type, data, size);
    }
    return 0; // Success
}

