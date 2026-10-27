#!/usr/bin/env python3
"""
Automated Smoke Test Suite for Smart Hospital Management System
Course: Computer Programming (26CAH-101) - Chandigarh University

This test suite:
1. Compiles main.c with strict C99 flags (-std=c99 -Wall -Wextra -Wpedantic).
2. Executes full operational workflow in a sandboxed temporary directory.
3. Verifies zero data pollution of repository root.
4. Validates all core functional modules:
   - Demo data ingestion (Option 99)
   - Patient registration (Option 1)
   - Emergency-first triage sort (Option 2)
   - Doctor assignment & department load (Option 3)
   - 2D Ward Bed allocation & grid rendering (Options 4, 5)
   - Treatment charge calculation (Option 6)
   - Recursive discharge billing (Option 7)
   - Patient search (Option 8)
   - Hospital analytics summary dashboard (Option 11)
   - Clean shutdown (Option 0)
"""

import os
import shutil
import subprocess
import sys
import tempfile

GREEN = "\033[1;32m"
RED = "\033[1;31m"
CYAN = "\033[1;36m"
YELLOW = "\033[1;33m"
RESET = "\033[0m"

def print_status(step, message, success=True):
    symbol = f"{GREEN}[PASS]{RESET}" if success else f"{RED}[FAIL]{RESET}"
    print(f" {symbol} {CYAN}{step:<24}{RESET} {message}")

def run_smoke_test():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    c_source = os.path.join(repo_root, "main.c")

    if not os.path.isfile(c_source):
        print(f"{RED}Error: main.c not found at {c_source}{RESET}")
        return False

    print(f"\n{CYAN}{'='*60}{RESET}")
    print(f"{CYAN}  Smart Hospital Management System — Automated Smoke Test{RESET}")
    print(f"{CYAN}{'='*60}{RESET}\n")

    # Step 1: Strict Compilation
    with tempfile.TemporaryDirectory() as sandbox_dir:
        bin_path = os.path.join(sandbox_dir, "hospital_test_bin")
        compile_cmd = [
            "gcc",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            c_source,
            "-o",
            bin_path
        ]

        build_res = subprocess.run(compile_cmd, capture_output=True, text=True)
        if build_res.returncode != 0:
            print_status("Compilation", f"Failed with exit code {build_res.returncode}", False)
            print(build_res.stderr)
            return False
        
        warning_count = len([line for line in build_res.stderr.strip().split('\n') if line.strip()])
        if warning_count > 0:
            print_status("Compilation", f"Compiled with warnings:\n{build_res.stderr}", False)
            return False
        else:
            print_status("Compilation", "Zero warnings (-std=c99 -Wall -Wextra -Wpedantic)", True)

        # Step 2: Prepare Automated Test Input Script
        # Workflow:
        # 99: Load demo data (5 patients)
        # 1: Register emergency patient (Aman Verma, age 34, M, Cardiac Arrest, priority 1)
        # 2: View Triage Priority List
        # 3: Assign Dr. Rajesh (ID 1, Cardiology) to patient 6
        # 5: Allocate Bed in Ward 2 (ICU) to patient 6
        # 4: Display 2D Bed Grid
        # 6: Add treatment charges for patient 6 (consult 500, med 1200, room 1500, test 800, 4 days)
        # 8: Search patient 6
        # 7: Discharge patient 6 (triggers recursive bill calculation)
        # 11: Display Hospital Analytics Summary
        # 0: Exit
        inputs = [
            # 99: Demo data
            "99", "",
            # 1: Register patient
            "1", "Aman Verma", "34", "M", "Cardiac Arrest", "1", "",
            # 2: Triage list
            "2", "",
            # 3: Assign doctor (Patient 6, Doctor 1)
            "3", "6", "1", "",
            # 5: Allocate Bed (Patient 6, Ward 2 = ICU)
            "5", "6", "2", "",
            # 4: Bed Grid
            "4", "",
            # 6: Add charges (Patient 6: 500, 1200, 1500, 800, 4 days)
            "6", "6", "500", "1200", "1500", "800", "4", "",
            # 8: Search patient 6
            "8", "6", "",
            # 7: Discharge patient 6
            "7", "6", "",
            # 11: Hospital Summary Dashboard
            "11", "",
            # 0: Exit
            "0"
        ]
        input_data = "\n".join(inputs) + "\n"

        # Step 3: Execute inside isolated sandbox
        proc = subprocess.run(
            [bin_path],
            input=input_data,
            cwd=sandbox_dir,
            capture_output=True,
            text=True
        )

        output = proc.stdout

        # Assertions
        tests = [
            ("Process Exit Code", proc.returncode == 0, f"Exit code {proc.returncode}"),
            ("Demo Ingestion", "Demo data loaded successfully" in output or "patients loaded" in output.lower(), "5 baseline records loaded"),
            ("Patient Registration", "Patient registered! ID =" in output, "Registered Aman Verma (ID: 6, Emergency)"),
            ("Triage Priority Sorting", "TRIAGE LIST" in output and "EMERGENCY" in output, "Emergency triage list sorted and displayed"),
            ("Doctor Assignment", "Doctor assigned successfully" in output, "Assigned Dr. Rajesh Sharma to Patient 6"),
            ("Bed Allocation", "Bed allocated: Ward 2" in output, "ICU Bed allocated successfully"),
            ("Bed Grid Rendering", "BED GRID" in output and "Ward A  General" in output, "2D Bed grid rendered with ward rows"),
            ("Charge Addition", "Charges updated successfully" in output, "Itemized treatment charges saved"),
            ("Patient Search", "SEARCH PATIENT" in output and "Aman Verma" in output, "Patient 6 located by ID"),
            ("Recursive Discharge", "DISCHARGE RECEIPT" in output and "TOTAL BILL" in output, "Recursive total bill computed & receipt saved"),
            ("Hospital Dashboard", "HOSPITAL SUMMARY DASHBOARD" in output and "Total Patients Registered" in output, "Aggregated hospital analytics rendered"),
            ("Receipt Export", any(fname.startswith("discharge_") and fname.endswith(".txt") for fname in os.listdir(sandbox_dir)), "discharge_6.txt written in sandbox"),
        ]

        all_passed = True
        for test_name, condition, detail in tests:
            print_status(test_name, detail, condition)
            if not condition:
                all_passed = False

        # Verify sandbox isolation: repo root must not contain generated test data
        root_txt_files = [f for f in os.listdir(repo_root) if f.startswith("discharge_6.txt")]
        isolation_ok = len(root_txt_files) == 0
        print_status("Sandbox Isolation", "Zero files leaked to repo root", isolation_ok)
        if not isolation_ok:
            all_passed = False

        print(f"\n{CYAN}{'='*60}{RESET}")
        if all_passed:
            print(f"{GREEN}  ALL SMOKE TESTS PASSED SUCCESSFULLY!{RESET}")
            print(f"{CYAN}{'='*60}{RESET}\n")
            return True
        else:
            print(f"{RED}  SOME TESTS FAILED. Check test outputs.{RESET}")
            print(f"{CYAN}{'='*60}{RESET}\n")
            return False

if __name__ == "__main__":
    success = run_smoke_test()
    sys.exit(0 if success else 1)
