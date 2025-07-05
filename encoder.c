#include "sctp.h"

/**
 * @file encoder.c
 * @brief Implements the Simple Compact Transaction Protocol (SCTP) encoder.
 *
 * This file contains the internal data structures and logic for serializing
 * various data types into a compact byte stream according to the SCTP format.
 * It is designed for use in WebAssembly environments and relies on a
 * pre-allocated buffer provided during initialization.
 */

// --- Internal Constants ---

#define SCTP_TYPE_MASK 0x0F
#define SCTP_META_MASK 0xF0
#define SCTP_META_SHIFT 4
#define SCTP_VECTOR_LARGE_FLAG 0x0F

// --- Internal Struct Definition ---

/**
 * @brief Holds the state of the SCTP encoder.
 *
 * This structure tracks the buffer, its total capacity, and the current
 * write position. It is not meant to be manipulated directly by the user.
 */
struct sctp_encoder {
    uint8_t* buffer;    ///< Pointer to the allocated memory buffer.
    size_t capacity;    ///< Total size of the buffer in bytes.
    size_t position;    ///< Current write offset in the buffer.
};

static sctp_encoder_t* g_encoder = NULL;

// --- Utility Functions ---

/**
 * @brief Ensures there is enough space in the buffer for additional data.
 *
 * If the requested capacity exceeds the buffer's limits, this function will
 * trigger an abort.
 *
 * @param enc A pointer to the encoder context.
 * @param additional_bytes The number of additional bytes required.
 */
static void _sctp_encoder_ensure_capacity(sctp_encoder_t* enc, size_t additional_bytes) {
    if (enc->position + additional_bytes > enc->capacity) {
        lea_abort("SCTP encoder out of capacity");
    }
}

/**
 * @brief Writes a single byte to the encoder's buffer.
 * @param enc A pointer to the encoder context.
 * @param byte The byte to write.
 */
static void _sctp_encoder_write_byte(sctp_encoder_t* enc, uint8_t byte) {
    _sctp_encoder_ensure_capacity(enc, 1);
    enc->buffer[enc->position++] = byte;
}

/**
 * @brief Writes an SCTP header byte (type and metadata).
 * @param enc A pointer to the encoder context.
 * @param type The SCTP data type.
 * @param meta The 4-bit metadata value.
 */
static void _sctp_encoder_write_header(sctp_encoder_t* enc, sctp_type_t type, uint8_t meta) {
    uint8_t header = (uint8_t)type | (meta << SCTP_META_SHIFT);
    _sctp_encoder_write_byte(enc, header);
}

/**
 * @brief Writes a block of raw data to the encoder's buffer.
 * @param enc A pointer to the encoder context.
 * @param data A pointer to the data to write.
 * @param size The size of the data in bytes.
 */
static void _sctp_encoder_write_data(sctp_encoder_t* enc, const void* data, size_t size) {
    _sctp_encoder_ensure_capacity(enc, size);
    memcpy(enc->buffer + enc->position, data, size);
    enc->position += size;
}

// --- LEB128 Encoding ---

/**
 * @brief Encodes a 64-bit unsigned integer using ULEB128 format.
 * @param enc A pointer to the encoder context.
 * @param value The value to encode.
 */
static void _sctp_encoder_write_uleb128(sctp_encoder_t* enc, uint64_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        _sctp_encoder_write_byte(enc, byte);
    } while (value != 0);
}

/**
 * @brief Encodes a 64-bit signed integer using SLEB128 format.
 * @param enc A pointer to the encoder context.
 * @param value The value to encode.
 */
static void _sctp_encoder_write_sleb128(sctp_encoder_t* enc, int64_t value) {
    bool more;
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        more = !((((value == 0) && ((byte & 0x40) == 0)) ||
                   ((value == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        _sctp_encoder_write_byte(enc, byte);
    } while (more);
}

// --- Encoder Public API Implementation ---

LEA_EXPORT(sctp_encoder_init)
void sctp_encoder_init(size_t capacity) {
    allocator_reset();
    g_encoder = malloc(sizeof(sctp_encoder_t));
    if (!g_encoder) lea_abort("malloc failed for sctp_encoder_t");

    g_encoder->buffer = malloc(capacity);
    if (!g_encoder->buffer) lea_abort("malloc failed for encoder buffer");
    g_encoder->capacity = capacity;
    g_encoder->position = 0;
}

LEA_EXPORT(sctp_encoder_data)
const uint8_t* sctp_encoder_data(void) {
    if (!g_encoder) lea_abort("encoder not initialized");
    return g_encoder->buffer;
}

LEA_EXPORT(sctp_encoder_size)
size_t sctp_encoder_size(void) {
    if (!g_encoder) lea_abort("encoder not initialized");
    return g_encoder->position;
}

LEA_EXPORT(sctp_encoder_add_vector)
void* sctp_encoder_add_vector(size_t length) {
    sctp_encoder_t* enc = g_encoder;
    if (!enc) lea_abort("encoder not initialized");
    if (length < SCTP_VECTOR_LARGE_FLAG) {
        _sctp_encoder_write_header(enc, SCTP_TYPE_VECTOR, (uint8_t)length);
    } else {
        _sctp_encoder_write_header(enc, SCTP_TYPE_VECTOR, SCTP_VECTOR_LARGE_FLAG);
        _sctp_encoder_write_uleb128(enc, length);
    }
    _sctp_encoder_ensure_capacity(enc, length);
    void* ptr = enc->buffer + enc->position;
    enc->position += length;
    return ptr;
}

LEA_EXPORT(sctp_encoder_add_short)
void sctp_encoder_add_short(uint8_t value) {
    if (!g_encoder) lea_abort("encoder not initialized");
    if (value > 15) {
        lea_abort("short value must be <= 15");
    }
    _sctp_encoder_write_header(g_encoder, SCTP_TYPE_SHORT, value);
}

/**
 * @def DEFINE_ENCODER_ADD_TYPE
 * @brief A macro to generate functions for adding fixed-size numeric types.
 *
 * This macro creates an exported function `sctp_encoder_add_NAME` that writes
 * the appropriate SCTP header and the binary representation of the value.
 *
 * @param name The suffix for the function name (e.g., int8, uint32).
 * @param type The C data type (e.g., int8_t, uint32_t).
 * @param sctp_type The corresponding `sctp_type_t` enum value.
 */
#define DEFINE_ENCODER_ADD_TYPE(name, type, sctp_type) \
LEA_EXPORT(sctp_encoder_add_##name) \
void sctp_encoder_add_##name(type value) { \
    if (!g_encoder) lea_abort("encoder not initialized"); \
    _sctp_encoder_write_header(g_encoder, sctp_type, 0); \
    _sctp_encoder_write_data(g_encoder, &value, sizeof(type)); \
}

DEFINE_ENCODER_ADD_TYPE(int8, int8_t, SCTP_TYPE_INT8)
DEFINE_ENCODER_ADD_TYPE(uint8, uint8_t, SCTP_TYPE_UINT8)
DEFINE_ENCODER_ADD_TYPE(int16, int16_t, SCTP_TYPE_INT16)
DEFINE_ENCODER_ADD_TYPE(uint16, uint16_t, SCTP_TYPE_UINT16)
DEFINE_ENCODER_ADD_TYPE(int32, int32_t, SCTP_TYPE_INT32)
DEFINE_ENCODER_ADD_TYPE(uint32, uint32_t, SCTP_TYPE_UINT32)
DEFINE_ENCODER_ADD_TYPE(int64, int64_t, SCTP_TYPE_INT64)
DEFINE_ENCODER_ADD_TYPE(uint64, uint64_t, SCTP_TYPE_UINT64)
DEFINE_ENCODER_ADD_TYPE(float32, float, SCTP_TYPE_FLOAT32)
DEFINE_ENCODER_ADD_TYPE(float64, double, SCTP_TYPE_FLOAT64)

LEA_EXPORT(sctp_encoder_add_uleb128)
void sctp_encoder_add_uleb128(uint64_t value) {
    if (!g_encoder) lea_abort("encoder not initialized");
    _sctp_encoder_write_header(g_encoder, SCTP_TYPE_ULEB128, 0);
    _sctp_encoder_write_uleb128(g_encoder, value);
}

LEA_EXPORT(sctp_encoder_add_sleb128)
void sctp_encoder_add_sleb128(int64_t value) {
    if (!g_encoder) lea_abort("encoder not initialized");
    _sctp_encoder_write_header(g_encoder, SCTP_TYPE_SLEB128, 0);
    _sctp_encoder_write_sleb128(g_encoder, value);
}

LEA_EXPORT(sctp_encoder_add_eof)
void sctp_encoder_add_eof(void) {
    if (!g_encoder) lea_abort("encoder not initialized");
    _sctp_encoder_write_header(g_encoder, SCTP_TYPE_EOF, 0);
}
