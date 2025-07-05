const { spawn } = require('child_process');
const FUZZ_ITERATIONS = 100;

let handled_crashes = 0;
let critical_crashes = 0;
let last_error = "None";

async function runFuzzer() {
    console.log(`--- Starting Fuzzer Runner for ${FUZZ_ITERATIONS} iterations ---`);
    console.log(`Last Error: ${last_error}`);

    for (let i = 0; i < FUZZ_ITERATIONS; i++) {
        process.stdout.write(`Iteration: ${i}/${FUZZ_ITERATIONS} | Handled: ${handled_crashes} | Critical: ${critical_crashes} | Last Error: ${last_error.substring(0, 80)}`);

        const child = spawn('node', ['fuzzer.js', '--child']);
        
        let stderr = '';
        child.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        await new Promise((resolve) => {
            child.on('close', (code) => {
                if (code === 1 && (stderr.includes('unreachable') || stderr.includes('ABORT'))) {
                    // This is an expected, handled crash from lea_abort()
                    handled_crashes++;
                    last_error = stderr.trim();
                } else if (code !== 0) {
                    // This is an unexpected or critical crash
                    critical_crashes++;
                    last_error = stderr.trim();
                    console.error(`\n\n--- 💥 CRITICAL CRASH DETECTED! 💥 ---`);
                    console.error(`Iteration: ${i}`);
                    console.error(`Exit code: ${code}`);
                    console.error(`Stderr: ${last_error}`);
                    // In a real scenario, you would save the input that caused this
                }
                resolve();
            });
        });
    }


    console.log(`\n\n--- Fuzzing Complete ---`);
    console.log(`Total Iterations: ${FUZZ_ITERATIONS}`);
    console.log(`Handled (Expected) Crashes: ${handled_crashes}`);
    console.log(`Critical (Unexpected) Crashes: ${critical_crashes}`);
    
    if (critical_crashes > 0) {
        console.error("\nWarning: Critical crashes were detected!");
        process.exit(1);
    } else {
        console.log("\nSuccess: No critical crashes found.");
    }
}

runFuzzer();