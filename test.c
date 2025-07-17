#include "sctp.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// This is required by the LEA environment when using printf/abort
#include <stdlea.h>

// When compiling the test, SCTP_HANDLER_PROVIDED should be defined in the
// makefile. This requires us to provide a dummy implementation.
void __sctp_data_handler(sctp_type_t type, const void *data, size_t size)
{
    (void)type;
    (void)data;
    (void)size;
    printf("[FAIL] TEST FAILED: __sctp_data_handler called unexpectedly!\n");
    LEA_ABORT();
}

static void assert_true(bool condition, const char *message)
{
    if (!condition)
    {
        printf("[FAIL] TEST FAILED: %s\n", message);
        LEA_ABORT();
    }
}

LEA_EXPORT(run_test) int run_test(void)
{
    printf(">> Starting SCTP integration test...\n");

    // --- 1. Encoding Phase ---
    printf("\n--- 1. Encoding Data ---\n");
    allocator_reset();
    sctp_encoder_init(256);

    const char *test_vector_data = "hello sctp";
    const size_t test_vector_size = strlen(test_vector_data);
    const int8_t val_i8 = -120;
    const uint16_t val_u16 = 65000;
    const int32_t val_i32 = -2000000000;
    const uint64_t val_u64 = 9000000000000000000ULL;
    const uint64_t val_uleb = 1234567890123ULL;
    const int64_t val_sleb = -9876543210987LL;
    const float val_f32 = 123.456f;
    const uint8_t val_short = 10;

    sctp_encoder_add_int8(val_i8);
    printf("   Encoded INT8:    %d\n", val_i8);
    sctp_encoder_add_uint16(val_u16);
    printf("   Encoded UINT16:  %u\n", val_u16);
    sctp_encoder_add_int32(val_i32);
    printf("   Encoded INT32:   %d\n", val_i32);
    sctp_encoder_add_uint64(val_u64);
    printf("   Encoded UINT64:  0x%llx\n", (unsigned long long)val_u64);
    sctp_encoder_add_uleb128(val_uleb);
    printf("   Encoded ULEB128: 0x%llx\n", (unsigned long long)val_uleb);
    sctp_encoder_add_sleb128(val_sleb);
    printf("   Encoded SLEB128: 0x%llx\n", (unsigned long long)val_sleb);
    sctp_encoder_add_float32(val_f32);
    printf("   Encoded FLOAT32: 0x%x (bits)\n", *(unsigned int *)&val_f32);
    sctp_encoder_add_short(val_short);
    printf("   Encoded SHORT:   %u\n", val_short);
    void *vec_ptr = sctp_encoder_add_vector(test_vector_size);
    memcpy(vec_ptr, test_vector_data, test_vector_size);
    printf("   Encoded VECTOR:  \"%*s\"\n", (int)test_vector_size, test_vector_data);
    sctp_encoder_add_eof();
    printf("   Encoded EOF\n");

    const uint8_t *encoded_data = sctp_encoder_data();
    const size_t encoded_size = sctp_encoder_size();

    printf("\n   Total encoded size: %u bytes.\n", (unsigned int)encoded_size);

    // --- 2. Decoding Phase ---
    printf("\n--- 2. Decoding & Verifying Data ---\n");
    sctp_decoder_t *dec = sctp_decoder_from_buffer(encoded_data, encoded_size);
    assert_true(dec != NULL, "Decoder creation failed.");

    // Field 1: INT8
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_INT8, "Type mismatch for INT8");
    assert_true(dec->last_value.as_int8 == val_i8, "Value mismatch for INT8");
    printf("   Decoded INT8:    %d\n", dec->last_value.as_int8);

    // Field 2: UINT16
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_UINT16, "Type mismatch for UINT16");
    assert_true(dec->last_value.as_uint16 == val_u16, "Value mismatch for UINT16");
    printf("   Decoded UINT16:  %u\n", dec->last_value.as_uint16);

    // Field 3: INT32
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_INT32, "Type mismatch for INT32");
    assert_true(dec->last_value.as_int32 == val_i32, "Value mismatch for INT32");
    printf("   Decoded INT32:   %d\n", dec->last_value.as_int32);

    // Field 4: UINT64
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_UINT64, "Type mismatch for UINT64");
    assert_true(dec->last_value.as_uint64 == val_u64, "Value mismatch for UINT64");
    printf("   Decoded UINT64:  0x%llx\n", (unsigned long long)dec->last_value.as_uint64);

    // Field 5: ULEB128
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_ULEB128, "Type mismatch for ULEB128");
    assert_true(dec->last_value.as_uleb128 == val_uleb, "Value mismatch for ULEB128");
    printf("   Decoded ULEB128: 0x%llx\n", (unsigned long long)dec->last_value.as_uleb128);

    // Field 6: SLEB128
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_SLEB128, "Type mismatch for SLEB128");
    assert_true(dec->last_value.as_sleb128 == val_sleb, "Value mismatch for SLEB128");
    printf("   Decoded SLEB128: 0x%llx\n", (unsigned long long)dec->last_value.as_sleb128);

    // Field 7: FLOAT32
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_FLOAT32, "Type mismatch for FLOAT32");
    assert_true(dec->last_value.as_float32 > 123.455f && dec->last_value.as_float32 < 123.457f,
                "Value mismatch for FLOAT32");
    printf("   Decoded FLOAT32: 0x%x (bits)\n", *(unsigned int *)&dec->last_value.as_float32);

    // Field 8: SHORT
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_SHORT, "Type mismatch for SHORT");
    assert_true(dec->last_value.as_short == val_short, "Value mismatch for SHORT");
    printf("   Decoded SHORT:   %u\n", dec->last_value.as_short);

    // Field 9: VECTOR
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_VECTOR, "Type mismatch for VECTOR");
    assert_true(dec->last_size == test_vector_size, "Size mismatch for VECTOR");
    assert_true(memcmp(dec->last_value.as_ptr, test_vector_data, test_vector_size) == 0, "Data mismatch for VECTOR");
    printf("   Decoded VECTOR:  \"%*s\"\n", (int)dec->last_size, (const char *)dec->last_value.as_ptr);

    // Field 10: EOF
    sctp_decoder_next(dec);
    assert_true(dec->last_type == SCTP_TYPE_EOF, "Expected EOF");
    printf("   Decoded EOF\n");

    printf("\n[OK] TEST PASSED\n");
    return 0;
}
