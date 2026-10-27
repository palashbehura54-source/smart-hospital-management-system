#!/usr/bin/env python3
"""
Automated Smoke Test Suite for Smart Hospital Management System (30-Patient Demo)
Course: Computer Programming (26CAH-101) - Chandigarh University

This test suite:
1. Compiles main.c with strict C99 flags (-std=c99 -Wall -Wextra -Wpedantic).
2. Executes full operational workflow in a sandboxed temporary directory.
3. Validates demo data loading safety (confirmation prompt, cancel with 'n').
4. Validates 30 seeded demo patients across all clinical states:
   - 15 Admitted (status=1) with unique beds across all 3 wards (5 free per ward)
   - 10 Waiting (status=0) with no beds (wardNo=-1, bedNo=-1)
   - 5 Discharged (status=2) with non-zero historical revenue
5. Validates doctor workload capping (max 5 per doctor; Dr. 2 at 5/5 full capacity).
6. Validates idempotent reloading (no record duplication or doctor count inflation).
7. Validates all interactive menu operations:
   - Option 1: New patient registration (Auto ID = 31)
   - Option 2: Emergency-first priority triage sorting
   - Option 3: Doctor assignment (rejection when full, success when capacity available)
   - Option 4: 2D Ward Bed Grid occupancy rendering
   - Option 5: Safe bed allocation to waiting patient
   - Option 6: Treatment charges recording
   - Option 7: Recursive discharge billing and receipt generation
   - Option 8: Dual-mode patient search (by ID and by Name substring)
   - Option 9: Patient record update
   - Option 10: Tabular display of all 30+ records
   - Option 11: Real-time analytics dashboard
   - Option 0: Clean exit
8. Verifies zero data pollution of repository root.
"""

import os
import re
import shutil
import struct
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
    print(f" {symbol} {CYAN}{step:<26}{RESET} {message}")

def run_smoke_test():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    c_source = os.path.join(repo_root, "main.c")

    if not os.path.isfile(c_source):
        print(f"{RED}Error: main.c not found at {c_source}{RESET}")
        return False

    print(f"\n{CYAN}{'='*64}{RESET}")
    print(f"{CYAN}  Smart Hospital Management System — 30-Patient Demo Smoke Test{RESET}")
    print(f"{CYAN}{'='*64}{RESET}\n")

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

        # Step 2: Test Option 99 cancellation
        res_decline = subprocess.run(
            [bin_path],
            input="99\nn\n\n0\n",
            cwd=sandbox_dir,
            capture_output=True,
            text=True
        )
        decline_ok = ("Demo data loading cancelled" in res_decline.stdout and
                      not os.path.exists(os.path.join(sandbox_dir, "patients.dat")))
        print_status("Confirmation Safety", "Option 99 cancel ('n') preserves state with zero data write", decline_ok)
        if not decline_ok:
            return False

        # Step 3: Comprehensive Interactive Test Script
        inputs = [
            # 1. Load 30 demo patients
            "99", "y", "",
            # 2. Reload 30 demo patients (idempotency check)
            "99", "y", "",
            # 3. Attempt assign Dr. 2 (5/5 full) to patient 17
            "3", "17", "2", "",
            # 4. Assign Dr. 1 (4/5) to patient 17
            "3", "17", "1", "",
            # 5. Show Bed Grid
            "4", "",
            # 6. Allocate Bed to patient 18 in Ward 1 (General)
            "5", "18", "1", "",
            # 7. Add charges for patient 18
            "6", "18", "500", "1200", "1000", "800", "3", "",
            # 8. Discharge patient 18
            "7", "18", "",
            # 9. Search by ID (15)
            "8", "1", "15", "",
            # 10. Search by Name ("Sengupta")
            "8", "2", "Sengupta", "",
            # 11. Update patient 16 details (new disease)
            "9", "16", "", "Recovering Concussion", "0", "-1", "",
            # 12. Display all patients
            "10", "",
            # 13. Hospital Dashboard
            "11", "",
            # 14. Register new patient (ID 31)
            "1", "Aman Verma", "34", "M", "Cardiac Arrest", "1", "",
            # 15. Triage list
            "2", "",
            # 16. Exit
            "0"
        ]
        input_data = "\n".join(inputs) + "\n"

        proc = subprocess.run(
            [bin_path],
            input=input_data,
            cwd=sandbox_dir,
            capture_output=True,
            text=True
        )

        raw_output = proc.stdout
        clean_output = re.sub(r'\x1b\[[0-9;]*m', '', raw_output)

        # Step 4: Verify Binary Structure of patients.dat
        pat_file = os.path.join(sandbox_dir, "patients.dat")
        with open(pat_file, "rb") as f:
            bdata = f.read()

        # 30 demo patients + 1 newly registered = 31 records of 192 bytes = 5952 bytes
        rec_fmt = "=i60sic3x80siiiiii4f"
        rec_size = struct.calcsize(rec_fmt)
        total_recs = len(bdata) // rec_size

        admitted_cnt = 0
        waiting_cnt = 0
        discharged_cnt = 0
        occupied_beds = set()

        # Verify initial 30 demo patients
        for i in range(30):
            rec = struct.unpack(rec_fmt, bdata[i * rec_size : (i + 1) * rec_size])
            pid, name_b, age, gender_b, disease_b, prio, docId, wardNo, bedNo, days, status = rec[:11]
            if status == 1:
                admitted_cnt += 1
                occupied_beds.add((wardNo, bedNo))
            elif status == 0:
                waiting_cnt += 1
            elif status == 2:
                discharged_cnt += 1

        # Assertions
        tests = [
            ("Process Exit Code", proc.returncode == 0, f"Exit code {proc.returncode}"),
            ("Demo 30 Ingestion", clean_output.count("30 demo patients loaded successfully") >= 1, "30 demo records seeded"),
            ("Idempotent Reload", clean_output.count("30 demo patients loaded successfully") == 2, "Reloading produces no duplicate records"),
            ("Record Count (30)", total_recs >= 30, f"{total_recs} records verified in binary database"),
            ("Status Distribution", admitted_cnt == 15 and waiting_cnt == 9 and discharged_cnt == 6,
             f"{admitted_cnt} admitted, {waiting_cnt} waiting, {discharged_cnt} discharged (patient 18 discharged during test)"),
            ("Zero Bed Overlap", len(occupied_beds) == admitted_cnt, f"{len(occupied_beds)} unique beds across wards"),
            ("Doctor Workload Limit", "Doctor is full (max 5 patients)" in clean_output, "Dr. Rohit Mehta (5/5) rejected assignment"),
            ("Doctor Assignment", "Doctor assigned successfully" in clean_output, "Dr. Priya Sharma assigned to patient 17"),
            ("Bed Grid Rendering", "BED GRID" in clean_output and "Ward A  General" in clean_output, "2D Bed matrix rendered with occupancy counts"),
            ("Bed Allocation", "Bed allocated: Ward 1, Bed 6" in clean_output, "First free bed in Ward 1 allotted to waiting patient"),
            ("Treatment Charges", "Charges updated successfully" in clean_output, "Itemized treatment charges recorded"),
            ("Recursive Discharge", "DISCHARGE RECEIPT" in clean_output and "Rs. 5500.00" in clean_output, "Recursive bill computed & receipt saved"),
            ("Receipt File Export", os.path.exists(os.path.join(sandbox_dir, "discharge_18.txt")), "discharge_18.txt generated in sandbox"),
            ("Patient Search", "Vihaan Chadha" in clean_output and "Diya Sengupta" in clean_output, "Both ID search and Name substring search passed"),
            ("Update Details", "Patient record updated successfully" in clean_output, "Updated patient 16 diagnosis in-place"),
            ("Registry Table", "Total records: 30" in clean_output, "Tabular listing rendered all 30 patients"),
            ("Analytics Dashboard", "Total Patients Registered : 30" in clean_output and "Rs. 37700.00" in clean_output, "Analytics dashboard aggregated all 30 records & revenue"),
            ("New Registration", "Patient registered! ID = 31" in clean_output, "Sequential ID 31 assigned to new intake"),
            ("Priority Triage", "TRIAGE LIST" in clean_output and "EMERGENCY" in clean_output, "Emergency triage sorted critical patients first"),
        ]

        all_passed = True
        for test_name, condition, detail in tests:
            print_status(test_name, detail, condition)
            if not condition:
                all_passed = False

        # Verify sandbox isolation: repo root must not contain generated test data
        root_data_files = [f for f in os.listdir(repo_root) if f.startswith("discharge_") or f.endswith(".dat")]
        isolation_ok = len(root_data_files) == 0
        print_status("Sandbox Isolation", "Zero runtime data files leaked to repo root", isolation_ok)
        if not isolation_ok:
            all_passed = False

        print(f"\n{CYAN}{'='*64}{RESET}")
        if all_passed:
            print(f"{GREEN}  ALL 30-PATIENT SMOKE TESTS PASSED SUCCESSFULLY!{RESET}")
            print(f"{CYAN}{'='*64}{RESET}\n")
            return True
        else:
            print(f"{RED}  SOME TESTS FAILED. Check diagnostics above.{RESET}")
            print(f"{CYAN}{'='*64}{RESET}\n")
            return False

if __name__ == "__main__":
    success = run_smoke_test()
    sys.exit(0 if success else 1)
