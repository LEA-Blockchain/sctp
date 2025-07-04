const fs = require('fs');
const assert = require('assert');

const encBuffer = fs.readFileSync('sctp.mvp.enc.wasm');
const decBuffer = fs.readFileSync('sctp.mvp.dec.wasm');

let encInstance, decInstance;
let encMemory, decMemory;

const sctp_type_enum = {
    SCTP_TYPE_INT8: 0, SCTP_TYPE_UINT8: 1,
    SCTP_TYPE_INT16: 2, SCTP_TYPE_UINT16: 3,
    SCTP_TYPE_INT32: 4, SCTP_TYPE_UINT32: 5,
    SCTP_TYPE_INT64: 6, SCTP_TYPE_UINT64: 7,
    SCTP_TYPE_ULEB128: 8, SCTP_TYPE_SLEB128: 9,
    SCTP_TYPE_FLOAT32: 10, SCTP_TYPE_FLOAT64: 11,
    SCTP_TYPE_SHORT: 12,
    SCTP_TYPE_VECTOR: 13,
    SCTP_TYPE_EOF: 15,
};

const decodedItems = [];
const importObject = {
    env: {
        __sctp_data_handler: (type, dataPtr, dataSize, userContext) => {
            let value;
            const dataView = new DataView(decMemory.buffer);
            switch (type) {
                case sctp_type_enum.SCTP_TYPE_INT32: value = dataView.getInt32(dataPtr, true); break;
                case sctp_type_enum.SCTP_TYPE_SHORT: value = new Uint8Array(decMemory.buffer, dataPtr, 1)[0]; break;
                case sctp_type_enum.SCTP_TYPE_VECTOR: value = new Uint8Array(decMemory.buffer, dataPtr, dataSize); break;
                case sctp_type_enum.SCTP_TYPE_UINT64: value = dataView.getBigUint64(dataPtr, true); break;
                case sctp_type_enum.SCTP_TYPE_EOF: value = 'EOF'; break;
                default: value = 'Unhandled Type';
            }
            decodedItems.push({ type, value });
        }
    }
};

async function runTest() {
    const { instance: encWasm } = await WebAssembly.instantiate(encBuffer, {});
    encInstance = encWasm;
    encMemory = encInstance.exports.memory;

    const { instance: decWasm } = await WebAssembly.instantiate(decBuffer, importObject);
    decInstance = decWasm;
    decMemory = decInstance.exports.memory;

    console.log('--- Running SCTP Test ---');

    // 1. Encoder
    const enc = encInstance.exports.sctp_encoder_init(1024);
    encInstance.exports.sctp_encoder_add_int32(enc, 123456);
    encInstance.exports.sctp_encoder_add_short(enc, 10);
    const testVector = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]);
    const vec_ptr = encInstance.exports.sctp_encoder_add_vector(enc, testVector.length);
    new Uint8Array(encMemory.buffer, vec_ptr, testVector.length).set(testVector);
    encInstance.exports.sctp_encoder_add_uint64(enc, 18446744073709551615n);
    encInstance.exports.sctp_encoder_add_eof(enc);
    const dataPtr = encInstance.exports.sctp_encoder_data(enc);
    const size = encInstance.exports.sctp_encoder_size(enc);
    const encodedData = new Uint8Array(encMemory.buffer, dataPtr, size).slice();
    console.log(`Encoded ${size} bytes.`);

    // 2. Decoder
    const decBufferPtr = decInstance.exports.__lea_malloc(size);
    new Uint8Array(decMemory.buffer, decBufferPtr, size).set(encodedData);
    const dec = decInstance.exports.sctp_decoder_init(decBufferPtr, size);
    const result = decInstance.exports.sctp_decoder_run(dec, 0);
    assert.strictEqual(result, 0, 'Decoder run successful');

    // 3. Verification
    console.log('Verifying decoded items...');
    assert.strictEqual(decodedItems.length, 5, 'Decoded 5 items');
    assert.strictEqual(decodedItems[0].value, 123456);
    assert.strictEqual(decodedItems[1].value, 10);
    assert.deepStrictEqual(decodedItems[2].value, testVector);
    assert.strictEqual(decodedItems[3].value, 18446744073709551615n);
    assert.strictEqual(decodedItems[4].value, 'EOF');
    console.log('--- Test Passed ---');
}

runTest().catch(err => {
    console.error("--- Test Failed ---");
    console.error(err);
    process.exit(1);
});