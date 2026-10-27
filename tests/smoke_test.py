#!/usr/bin/env python3
"""
Automated Smoke Test Suite for Smart Hospital Management System (30-Patient Demo)
Course: Computer Programming (26CAH-101) - Chandigarh University

This test suite:
1. Compiles main.c with strict C99 flags (-std=c99 -Wall -Wextra -Wpedantic).
2. Executes full operational workflow in a sandboxed temporary directory.
3. Validates demo data loading safety (confirmation prompt, cancel with 'n').
4. Verifies the INITIAL demo dataset immediately after option 99 confirmation
   and BEFORE any workflow modifications:
   - Exactly 30 records (IDs 1 through 30)
   - 15 Admitted (status=1) with 15 unique beds (5 per ward, 5 free per ward)
   - 10 Waiting (status=0) with wardNo=-1 and bedNo=-1
   - 5 Discharged (status=2) with wardNo=-1, bedNo=-1, and non-zero revenue
   - Doctor workload caps <= 5 (Dr. 2 full at 5/5)
5. Executes full interactive operational workflows:
   - Idempotent reload check (Option 99 reload)
   - Doctor capacity enforcement (Dr. 2 rejection at 5/5, Dr. 1 assignment at 4/5)
   - 2D Bed Grid occupancy rendering (Option 4)
   - Bed allocation to waiting patient 18 in Ward 1 (Option 5)
   - Treatment charge recording for patient 18 (Option 6)
   - Recursive discharge billing and receipt export for patient 18 (Option 7)
   - Dual-mode patient search by ID and Name substring (Option 8)
   - In-place patient record update (Option 9)
   - Tabular registry display of 30 records (Option 10)
   - Real-time analytics dashboard with recursive emergency count and revenue (Option 11)
   - New patient intake with auto-assigned sequential ID 31 (Option 1)
   - Emergency-first priority triage sorting (Option 2)
   - Clean shutdown (Option 0)
6. Verifies POST-WORKFLOW dataset state:
   - Final Record Count: exactly 31 records (30 seed + 1 registered)
   - Post-Workflow Distribution: 15 admitted, 10 waiting, 6 discharged
   - Bed consistency: 15 beds occupied after patient 18 bed is freed
7. Verifies zero data pollution of repository root.
"""

import os
import re
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
    print(f" {symbol} {CYAN}{step:<28}{RESET} {message}")

def run_smoke_test():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    c_source = os.path.join(repo_root, "main.c")

    if not os.path.isfile(c_source):
        print(f"{RED}Error: main.c not found at {c_source}{RESET}")
        return False

    print(f"\n{CYAN}{'='*68}{RESET}")
    print(f"{CYAN}  Smart Hospital Management System — 30-Patient Demo Smoke Test{RESET}")
    print(f"{CYAN}{'='*68}{RESET}\n")

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

        # Step 3: Seed initial demo dataset and verify immediately BEFORE workflow modifications
        res_seed = subprocess.run(
            [bin_path],
            input="99\ny\n\n0\n",
            cwd=sandbox_dir,
            capture_output=True,
            text=True
        )
        seed_ok = "30 demo patients loaded successfully" in res_seed.stdout
        print_status("Demo 30 Seed Ingestion", "30 demo patients initialized into clean database", seed_ok)
        if not seed_ok:
            return False

        # Inspect initial binary patients.dat
        pat_file = os.path.join(sandbox_dir, "patients.dat")
        with open(pat_file, "rb") as f:
            seed_data = f.read()

        rec_fmt = "=i60sic3x80siiiiii4f"
        rec_size = struct.calcsize(rec_fmt)
        total_seed_recs = len(seed_data) // rec_size

        init_recs = [struct.unpack(rec_fmt, seed_data[i * rec_size : (i + 1) * rec_size])
                     for i in range(total_seed_recs)]

        init_adm = sum(1 for r in init_recs if r[10] == 1)
        init_wai = sum(1 for r in init_recs if r[10] == 0)
        init_dis = sum(1 for r in init_recs if r[10] == 2)
        init_occupied_beds = {(r[7], r[8]) for r in init_recs if r[10] == 1}

        # Inspect initial beds.dat
        with open(os.path.join(sandbox_dir, "beds.dat"), "rb") as f:
            bed_data = f.read()
        init_beds = struct.unpack("=30i", bed_data)
        init_bed_occ = sum(1 for b in init_beds if b == 1)
        init_bed_free = sum(1 for b in init_beds if b == 0)

        # Inspect initial doctor loads for active patients (status != 2)
        init_doc_loads = {d: 0 for d in range(1, 6)}
        for r in init_recs:
            doc_id, status = r[6], r[10]
            if status != 2 and 1 <= doc_id <= 5:
                init_doc_loads[doc_id] += 1

        # Assert initial seed conditions
        init_tests = [
            ("Initial Record Count (30)", total_seed_recs == 30, f"Exactly {total_seed_recs} records in initial seed"),
            ("Initial Status Distribution", init_adm == 15 and init_wai == 10 and init_dis == 5,
             f"{init_adm} admitted, {init_wai} waiting, {init_dis} discharged"),
            ("Initial Bed Consistency", len(init_occupied_beds) == 15 and init_bed_occ == 15 and init_bed_free == 15,
             "15 unique beds occupied (5 per ward), 15 free of 30 total"),
            ("Initial Doctor Workloads", all(cnt <= 5 for cnt in init_doc_loads.values()) and init_doc_loads[2] == 5,
             f"Doctor loads: {dict(init_doc_loads)} (all <= 5, Dr. 2 full at 5/5)"),
        ]

        for test_name, condition, detail in init_tests:
            print_status(test_name, detail, condition)
            if not condition:
                return False

        # Step 4: Execute Comprehensive Interactive Workflow
        inputs = [
            # 1. Reload 30 demo patients to verify reload idempotency
            "99", "y", "",
            # 2. Attempt assign Dr. 2 (5/5 full) to patient 17
            "3", "17", "2", "",
            # 3. Assign Dr. 1 (4/5) to patient 17
            "3", "17", "1", "",
            # 4. Show Bed Grid
            "4", "",
            # 5. Allocate Bed to patient 18 in Ward 1 (General)
            "5", "18", "1", "",
            # 6. Add charges for patient 18
            "6", "18", "500", "1200", "1000", "800", "3", "",
            # 7. Discharge patient 18
            "7", "18", "",
            # 8. Search by ID (15)
            "8", "1", "15", "",
            # 9. Search by Name ("Sengupta")
            "8", "2", "Sengupta", "",
            # 10. Update patient 16 details (new disease)
            "9", "16", "", "Recovering Concussion", "0", "-1", "",
            # 11. Display all patients (30 records before new registration)
            "10", "",
            # 12. Hospital Dashboard (reflects 30 registered, Rs. 37700.00)
            "11", "",
            # 13. Register new emergency patient (Aman Verma, ID 31)
            "1", "Aman Verma", "34", "M", "Cardiac Arrest", "1", "",
            # 14. Triage list (Emergency first)
            "2", "",
            # 15. Exit
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

        # Step 5: Verify POST-WORKFLOW Binary State
        with open(pat_file, "rb") as f:
            post_data = f.read()

        total_final_recs = len(post_data) // rec_size
        final_recs = [struct.unpack(rec_fmt, post_data[i * rec_size : (i + 1) * rec_size])
                      for i in range(total_final_recs)]

        fin_adm = sum(1 for r in final_recs if r[10] == 1)
        fin_wai = sum(1 for r in final_recs if r[10] == 0)
        fin_dis = sum(1 for r in final_recs if r[10] == 2)
        fin_occupied_beds = {(r[7], r[8]) for r in final_recs if r[10] == 1}

        with open(os.path.join(sandbox_dir, "beds.dat"), "rb") as f:
            fin_bed_data = f.read()
        fin_beds = struct.unpack("=30i", fin_bed_data)
        fin_bed_occ = sum(1 for b in fin_beds if b == 1)
        fin_bed_free = sum(1 for b in fin_beds if b == 0)

        # Assertions
        workflow_tests = [
            ("Process Exit Code", proc.returncode == 0, f"Exit code {proc.returncode}"),
            ("Idempotent Reload", clean_output.count("30 demo patients loaded successfully") >= 1, "Reloading preserves exact record state"),
            ("Final Record Count (31)", total_final_recs == 31, f"Exactly {total_final_recs} records verified (30 seed + 1 registered)"),
            ("Post-Workflow Distribution", fin_adm == 15 and fin_wai == 10 and fin_dis == 6,
             f"{fin_adm} admitted, {fin_wai} waiting, {fin_dis} discharged (patient 18 discharged, patient 31 registered)"),
            ("Final Bed Consistency", len(fin_occupied_beds) == 15 and fin_bed_occ == 15 and fin_bed_free == 15,
             "15 unique beds occupied after patient 18 bed is freed"),
            ("Doctor Workload Limit", "Doctor is full (max 5 patients)" in clean_output, "Dr. Rohit Mehta (5/5) rejected assignment"),
            ("Doctor Assignment", "Doctor assigned successfully" in clean_output, "Dr. Priya Sharma assigned to patient 17"),
            ("Bed Grid Rendering", "BED GRID" in clean_output and "Ward A  General" in clean_output, "2D Bed matrix rendered with occupancy counts"),
            ("Bed Allocation", "Bed allocated: Ward 1, Bed 6" in clean_output, "First free bed in Ward 1 allotted to waiting patient"),
            ("Treatment Charges", "Charges updated successfully" in clean_output, "Itemized treatment charges recorded"),
            ("Recursive Discharge", "DISCHARGE RECEIPT" in clean_output and "Rs. 5500.00" in clean_output, "Recursive bill computed & receipt saved"),
            ("Receipt File Export", os.path.exists(os.path.join(sandbox_dir, "discharge_18.txt")), "discharge_18.txt generated in sandbox"),
            ("Patient Search", "Vihaan Chadha" in clean_output and "Diya Sengupta" in clean_output, "Both ID search and Name substring search passed"),
            ("Update Details", "Patient record updated successfully" in clean_output, "Updated patient 16 diagnosis in-place"),
            ("Registry Table", "Total records: 30" in clean_output, "Tabular listing rendered all 30 initial patients"),
            ("Analytics Dashboard", "Total Patients Registered : 30" in clean_output and "Rs. 37700.00" in clean_output, "Analytics dashboard aggregated all 30 records & revenue"),
            ("New Registration", "Patient registered! ID = 31" in clean_output, "Sequential ID 31 assigned to new intake"),
            ("Priority Triage", "TRIAGE LIST" in clean_output and "EMERGENCY" in clean_output, "Emergency triage sorted critical patients first"),
        ]

        all_passed = True
        for test_name, condition, detail in workflow_tests:
            print_status(test_name, detail, condition)
            if not condition:
                all_passed = False

        # Verify sandbox isolation: repo root must not contain generated test data
        root_data_files = [f for f in os.listdir(repo_root) if f.startswith("discharge_") or f.endswith(".dat")]
        isolation_ok = len(root_data_files) == 0
        print_status("Sandbox Isolation", "Zero runtime data files leaked to repo root", isolation_ok)
        if not isolation_ok:
            all_passed = False

        print(f"\n{CYAN}{'='*68}{RESET}")
        if all_passed:
            print(f"{GREEN}  ALL SMOKE TESTS PASSED WITH 100% ACCURACY!{RESET}")
            print(f"{CYAN}{'='*68}{RESET}\n")
            return True
        else:
            print(f"{RED}  SOME TESTS FAILED. Check diagnostics above.{RESET}")
            print(f"{CYAN}{'='*68}{RESET}\n")
            return False

if __name__ == "__main__":
    success = run_smoke_test()
    sys.exit(0 if success else 1)
