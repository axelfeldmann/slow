#!/usr/bin/env python3

import argparse
import subprocess

def parse_perf_output(output):
    
    lines = output.split("\n")
    
    cycles = None
    instructions = None
    
    for line in lines:
        if "cycles" in line:
            cycles = int(line.split()[0].replace(",", ""))
        elif "instructions" in line:
            instructions = int(line.split()[0].replace(",", ""))
        
    if cycles and instructions:
        return cycles / instructions
    else:
        return None

def main():
    parser = argparse.ArgumentParser(description="Run perf stat on a given PID.")
    parser.add_argument("pid", type=int, help="Process ID to monitor with perf stat")
    args = parser.parse_args()

    cmd = ["perf", "stat", "-e", "cycles,instructions", "-p", str(args.pid)]

    try:
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        _, stderr = process.communicate()
        process.wait()

    except KeyboardInterrupt:

        _, stderr = process.communicate()

        process.terminate()
        process.wait()

        output = stderr.decode("utf-8")
        cpi = parse_perf_output(output)
        
        if cpi:
            print(f"\n{cpi = :.2f}")
        else:
            print("Error parsing the output.")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
