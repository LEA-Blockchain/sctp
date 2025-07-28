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

#include <stdbool.h>

#define SCTP_TYPE_MASK 0x0F
#define SCTP_META_MASK 0xF0
#define SCTP_META_SHIFT 4
#define SCTP_VECTOR_LARGE_FLAG 0x0F

// --- LEB128 Decoding ---

/**
 * @brief Reads a single byte from the decoder's stream.
 * @param dec A pointer to the decoder context.
 * @param byte A pointer to a uint8_t to store the read byte.
 * @return 0 on success, -1 on end-of-file.
 */
static int _sctp_decoder_read_byte(sctp_decoder_t *dec, uint8_t *byte)
{
    if (dec->position >= dec->size)
    {
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
static const void *_sctp_decoder_read_data(sctp_decoder_t *dec, size_t size)
{
    if (dec->position + size > dec->size)
    {
        LEA_ABORT();
        return NULL;
    }
    const void *ptr = dec->data + dec->position;
    dec->position += size;
    return ptr;
}

/**
 * @brief Decodes a 64-bit unsigned integer from the stream using ULEB128 format.
 * @param dec A pointer to the decoder context.
 * @return The decoded uint64_t value.
 * @note Aborts if the stream ends unexpectedly or if an overflow occurs.
 */
static uint64_t _sctp_decoder_read_uleb128(sctp_decoder_t *dec)
{
    uint64_t result = 0;
    uint8_t shift = 0;
    uint8_t byte;
    while (1)
    {
        if (_sctp_decoder_read_byte(dec, &byte) != 0)
            LEA_ABORT();
        result |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
        {
            break;
        }
        shift += 7;
        if (shift >= 64)
            LEA_ABORT();
    }
    return result;
}

/**
 * @brief Decodes a 64-bit signed integer from the stream using SLEB128 format.
 * @param dec A pointer to the decoder context.
 * @return The decoded int64_t value.
 * @note Aborts if the stream ends unexpectedly or if an overflow occurs.
 */
static int64_t _sctp_decoder_read_sleb128(sctp_decoder_t *dec)
{
    int64_t result = 0;
    uint8_t shift = 0;
    uint8_t byte;
    while (1)
    {
        if (_sctp_decoder_read_byte(dec, &byte) != 0)
            LEA_ABORT();
        result |= (int64_t)(byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0)
        {
            if ((shift < 64) && (byte & 0x40))
            {
                result |= -((int64_t)1 << shift);
            }
            break;
        }
        if (shift >= 64)
            LEA_ABORT();
    }
    return result;
}

/**
 * @brief Declaration of the imported host function for handling decoded data.
 * @see sctp_data_handler_t
 */
#ifdef SCTP_CALLBACK_ENABLE
#ifndef SCTP_HANDLER_PROVIDED
LEA_IMPORT(env, __sctp_data_handler)
#endif
void __sctp_data_handler(sctp_type_t type, const void *data, size_t size);
#endif

// --- Decoder Public API Implementation ---

LEA_EXPORT(sctp_decoder_init)
sctp_decoder_t *sctp_decoder_init(size_t size)
{
    allocator_reset();
    sctp_decoder_t *dec = malloc(sizeof(sctp_decoder_t));
    if (!dec)
        LEA_ABORT();

    void *buffer = malloc(size);
    if (!buffer)
        LEA_ABORT();

    dec->data = buffer;
    dec->size = size;
    dec->position = 0;
    dec->last_type = SCTP_TYPE_EOF;
    dec->last_size = 0;
    memset(&dec->last_value, 0, sizeof(sctp_value_t));
    dec->is_external_buffer = false;

    return dec;
}

LEA_EXPORT(sctp_decoder_from_buffer)
sctp_decoder_t *sctp_decoder_from_buffer(const void *buffer, size_t size)
{
    sctp_decoder_t *dec = malloc(sizeof(sctp_decoder_t));
    if (!dec)
        LEA_ABORT();

    dec->data = buffer;
    dec->size = size;
    dec->position = 0;
    dec->last_type = SCTP_TYPE_EOF;
    dec->last_size = 0;
    memset(&dec->last_value, 0, sizeof(sctp_value_t));
    dec->is_external_buffer = true;

    return dec;
}

LEA_EXPORT(sctp_decoder_get_buffer)
void *sctp_decoder_get_buffer(sctp_decoder_t *dec)
{
    if (!dec || dec->is_external_buffer)
    {
        return NULL;
    }
    // Safe to cast away const because we know this buffer was created
    // for writing by sctp_decoder_init.
    return (void *)dec->data;
}

LEA_EXPORT(sctp_decoder_next)
sctp_type_t sctp_decoder_next(sctp_decoder_t *dec)
{
    if (!dec)
        LEA_ABORT();

    if (dec->position >= dec->size)
    {
        dec->last_type = SCTP_TYPE_EOF;
        dec->last_size = 0;
        return SCTP_TYPE_EOF;
    }

    uint8_t header;
    if (_sctp_decoder_read_byte(dec, &header) != 0)
    {
        dec->last_type = SCTP_TYPE_EOF;
        dec->last_size = 0;
        return SCTP_TYPE_EOF; // Clean EOF
    }

    sctp_type_t type = (sctp_type_t)(header & SCTP_TYPE_MASK);
    uint8_t meta = (header & SCTP_META_MASK) >> SCTP_META_SHIFT;

    dec->last_type = type;

    switch (type)
    {
    case SCTP_TYPE_INT8:
        dec->last_size = 1;
        dec->last_value.as_int8 = *(int8_t *)_sctp_decoder_read_data(dec, 1);
        break;
    case SCTP_TYPE_UINT8:
        dec->last_size = 1;
        dec->last_value.as_uint8 = *(uint8_t *)_sctp_decoder_read_data(dec, 1);
        break;
    case SCTP_TYPE_INT16:
        dec->last_size = 2;
        dec->last_value.as_int16 = *(int16_t *)_sctp_decoder_read_data(dec, 2);
        break;
    case SCTP_TYPE_UINT16:
        dec->last_size = 2;
        dec->last_value.as_uint16 = *(uint16_t *)_sctp_decoder_read_data(dec, 2);
        break;
    case SCTP_TYPE_INT32:
        dec->last_size = 4;
        dec->last_value.as_int32 = *(int32_t *)_sctp_decoder_read_data(dec, 4);
        break;
    case SCTP_TYPE_UINT32:
        dec->last_size = 4;
        dec->last_value.as_uint32 = *(uint32_t *)_sctp_decoder_read_data(dec, 4);
        break;
    case SCTP_TYPE_INT64:
        dec->last_size = 8;
        dec->last_value.as_int64 = *(int64_t *)_sctp_decoder_read_data(dec, 8);
        break;
    case SCTP_TYPE_UINT64:
        dec->last_size = 8;
        dec->last_value.as_uint64 = *(uint64_t *)_sctp_decoder_read_data(dec, 8);
        break;
    case SCTP_TYPE_FLOAT32:
        dec->last_size = 4;
        dec->last_value.as_float32 = *(float *)_sctp_decoder_read_data(dec, 4);
        break;
    case SCTP_TYPE_FLOAT64:
        dec->last_size = 8;
        dec->last_value.as_float64 = *(double *)_sctp_decoder_read_data(dec, 8);
        break;
    case SCTP_TYPE_ULEB128:
        dec->last_value.as_uleb128 = _sctp_decoder_read_uleb128(dec);
        dec->last_size = sizeof(uint64_t);
        break;
    case SCTP_TYPE_SLEB128:
        dec->last_value.as_sleb128 = _sctp_decoder_read_sleb128(dec);
        dec->last_size = sizeof(int64_t);
        break;
    case SCTP_TYPE_SHORT:
        dec->last_value.as_short = meta;
        dec->last_size = 1;
        break;
    case SCTP_TYPE_VECTOR:
        if (meta == SCTP_VECTOR_LARGE_FLAG)
        {
            dec->last_size = (size_t)_sctp_decoder_read_uleb128(dec);
        }
        else
        {
            dec->last_size = meta;
        }
        dec->last_value.as_ptr = _sctp_decoder_read_data(dec, dec->last_size);
        break;
    case SCTP_TYPE_EOF:
        dec->last_size = 0;
        break;
    default:
        LEA_ABORT();
    }
    return type;
}

#ifdef SCTP_CALLBACK_ENABLE
LEA_EXPORT(sctp_decoder_run)
int sctp_decoder_run(sctp_decoder_t *dec)
{
    if (!dec)
        LEA_ABORT();

    while (sctp_decoder_next(dec) != SCTP_TYPE_EOF)
    {
        const void *data_ptr;
        // For pointer types, use the pointer directly.
        // For value types, use the address of the value in the union.
        switch (dec->last_type)
        {
        case SCTP_TYPE_VECTOR:
            data_ptr = dec->last_value.as_ptr;
            break;
        case SCTP_TYPE_ULEB128:
            data_ptr = &dec->last_value.as_uleb128;
            break;
        case SCTP_TYPE_SLEB128:
            data_ptr = &dec->last_value.as_sleb128;
            break;
        // For all other types, we can take the address of the union member
        default:
            data_ptr = &dec->last_value;
            break;
        }
        __sctp_data_handler(dec->last_type, data_ptr, dec->last_size);
    }
    // Final EOF callback
    __sctp_data_handler(SCTP_TYPE_EOF, NULL, 0);

    return 0; // Success
}
#endif

