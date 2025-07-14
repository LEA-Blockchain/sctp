const fs = require('fs').promises;
const path = require('path');

async function main() {
    const wasmPath = process.argv[2];
    if (!wasmPath) {
        console.error('Error: Please provide the path to the test.wasm file.');
        console.error('Usage: node test.js <path/to/test.wasm>');
        process.exit(1);
    }
    console.log(`[INFO] Loading test module from: ${wasmPath}`);

    let memory;

    // Host functions for the WASM module.
    const importObject = {
        env: {
            // Called by `lea_abort` in C.
            __lea_log: (ptr) => {
                if (!memory) return;
                const mem = new Uint8Array(memory.buffer);
                const end = mem.indexOf(0, ptr);
                const message = new TextDecoder('utf-8').decode(mem.subarray(ptr, end));
                console.error(message);
            },
            // Called by `lea_printf` in C.
            __lea_log2: (ptr, len) => {
                if (!memory) return;
                const _len = Number(len);
                const mem = new Uint8Array(memory.buffer, ptr, _len);
                const message = new TextDecoder('utf-8').decode(mem);
                process.stdout.write(message);
            }
        },
    };

    try {
        const wasmBytes = await fs.readFile(wasmPath);
        const { instance } = await WebAssembly.instantiate(wasmBytes, importObject);

        memory = instance.exports.memory;

        const { run_test } = instance.exports;
        if (typeof run_test !== 'function') {
            throw new Error("The 'run_test' function is not exported from the WASM module.");
        }

        console.log('[INFO] Running test...\n---------------------------------');
        const exitCode = run_test();
        console.log('---------------------------------');

        if (exitCode === 0) {
            console.log('[PASS] Test run completed successfully.');
            process.exit(0);
        } else {
            console.error(`[FAIL] Test run failed with exit code: ${exitCode}`);
            process.exit(1);
        }
    } catch (e) {
        // A "unreachable" error means the WASM module aborted (a test failure).
        console.error(`\n[FAIL] An error occurred during the test run:`);
        console.error(e.message.includes('unreachable') 
            ? 'WASM module executed an abort (trap), indicating a test failure.' 
            : e
        );
        process.exit(1);
    }
}

main().catch(e => {
    console.error(e);
    process.exit(1);
});
