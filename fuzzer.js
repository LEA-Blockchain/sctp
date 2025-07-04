const fs = require('fs');
const assert = require('assert');

// --- Fuzzer Configuration ---
const FUZZ_ITERATIONS = 100000; // Number of fuzzing inputs to generate
const MAX_INPUT_SIZE = 30000;   // Max size of a single fuzz input
const VERBOSE = false;         // Set to true to log every generated input

// --- Load WASM Module ---
const decBuffer = fs.readFileSync('sctp.mvp.dec.wasm');
let decInstance;
let decMemory;

// --- SCTP Type Definitions (for the generator) ---
const sctp_type_enum = {
    SCTP_TYPE_INT8: 0, SCTP_TYPE_UINT8: 1,
    SCTP_TYPE_INT16: 2, SCTP_TYPE_UINT16: 3,
    SCTP_TYPE_INT32: 4, SCTP_TYPE_UINT32: 5,
    SCTP_TYPE_INT64: 6, SCTP_TYPE_UINT64: 7,
    SCTP_TYPE_ULEB128: 8, SCTP_TYPE_SLEB128: 9,
    SCTP_TYPE_FLOAT32: 10, SCTP_TYPE_FLOAT64: 11,
    SCTP_TYPE_SHORT: 12,
    SCTP_TYPE_VECTOR: 13,
    SCTP_TYPE_RESERVED: 14,
    SCTP_TYPE_EOF: 15,
};
const ALL_TYPES = Object.values(sctp_type_enum);

// --- Fuzz Data Generator ---

// Helper to write a ULEB128 value to a buffer
function writeUleb128(buffer, value) {
    do {
        let byte = value & 0x7f;
        value >>= 7;
        if (value !== 0) {
            byte |= 0x80;
        }
        buffer.push(byte);
    } while (value !== 0);
}

// Main generator function
function generateFuzzData() {
    const buffer = [];
    const numFields = Math.floor(Math.random() * 20) + 1;

    for (let i = 0; i < numFields; i++) {
        const type = ALL_TYPES[Math.floor(Math.random() * ALL_TYPES.length)];
        const meta = Math.floor(Math.random() * 16);
        const header = type | (meta << 4);
        buffer.push(header);

        // Based on type, add potentially malformed data
        const mutation = Math.random();
        
        if (type === sctp_type_enum.SCTP_TYPE_VECTOR) {
            if (meta === 15) { // Large vector
                if (mutation < 0.33) {
                    // Malformed ULEB: overly long
                    for(let j=0; j<12; j++) buffer.push(0x80);
                    buffer.push(0x00);
                } else if (mutation < 0.66) {
                    // Length points past where the buffer will end
                    writeUleb128(buffer, MAX_INPUT_SIZE * 2);
                } else {
                    // Normal-ish case
                    writeUleb128(buffer, Math.floor(Math.random() * 50));
                }
            } else { // Small vector
                 if (mutation < 0.5) {
                    // Data length mismatch (less data than header claims)
                    for(let j=0; j < meta -1; j++) buffer.push(Math.floor(Math.random() * 256));
                 }
            }
        } else if (type >= sctp_type_enum.SCTP_TYPE_INT8 && type <= sctp_type_enum.SCTP_TYPE_FLOAT64) {
             if (mutation < 0.5) {
                // Truncated data
                buffer.push(1, 2, 3);
             }
        }
    }
    
    // Add random bytes to the end
    const randomBytes = Math.floor(Math.random() * 10);
    for(let i=0; i<randomBytes; i++) {
        buffer.push(Math.floor(Math.random() * 256));
    }

    return new Uint8Array(buffer.slice(0, MAX_INPUT_SIZE));
}


// --- Fuzzer Execution ---

const importObject = {
    env: {
        // A dummy handler. We only care if the decoder *crashes*, not what it produces.
        __sctp_data_handler: () => {}
    }
};

async function runFuzzIteration() {
    const { instance } = await WebAssembly.instantiate(decBuffer, importObject);
    decInstance = instance;
    decMemory = decInstance.exports.memory;

    const fuzzInput = generateFuzzData();

    const bufferPtr = decInstance.exports.__lea_malloc(fuzzInput.length);
    new Uint8Array(decMemory.buffer, bufferPtr, fuzzInput.length).set(fuzzInput);

    const dec = decInstance.exports.sctp_decoder_init(bufferPtr, fuzzInput.length);

    try {
        decInstance.exports.sctp_decoder_run(dec, 0);
    } catch (e) {
        // Propagate the error to the runner
        console.error(e.message);
        process.exit(1);
    }
}

// Only run the fuzzer if executed as a child process
if (process.argv.includes('--child')) {
    runFuzzIteration().catch(err => {
        console.error(err);
        process.exit(1);
    });
}
