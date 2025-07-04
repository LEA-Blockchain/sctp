const { spawn } = require('child_process');
const FUZZ_ITERATIONS = 10000;

let handled_crashes = 0;
let critical_crashes = 0;

async function runFuzzer() {
    console.log(`--- Starting Fuzzer Runner for ${FUZZ_ITERATIONS} iterations ---`);

    for (let i = 0; i < FUZZ_ITERATIONS; i++) {
        if (i % 100 === 0) {
            process.stdout.write(`Iteration: ${i}/${FUZZ_ITERATIONS} | Handled Crashes: ${handled_crashes} | Critical Crashes: ${critical_crashes}\r`);
        }

        const child = spawn('node', ['fuzzer.js', '--child']);
        
        let stderr = '';
        child.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        await new Promise((resolve) => {
            child.on('close', (code) => {
                if (code === 1 && stderr.includes('unreachable')) {
                    // This is an expected, handled crash from lea_abort()
                    handled_crashes++;
                } else if (code !== 0) {
                    // This is an unexpected or critical crash
                    critical_crashes++;
                    console.error(`\n\n--- 💥 CRITICAL CRASH DETECTED! 💥 ---`);
                    console.error(`Iteration: ${i}`);
                    console.error(`Exit code: ${code}`);
                    console.error(`Stderr: ${stderr}`);
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