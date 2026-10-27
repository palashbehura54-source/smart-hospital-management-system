# 🏥 Smart Hospital Management System with Emergency Triage

> **Course:** Computer Programming (26CAH-101) | **Institution:** Chandigarh University, Mohali, Punjab  
> **Author:** Palash Behura | **Language:** C (Standard C99)

A comprehensive, console-based clinical workflow and emergency triage system built strictly within foundational C programming syllabus boundaries. Features include priority-based emergency triage queueing, 2D matrix bed allocation across hospital wards, doctor workload caps, in-place binary file persistence, recursive billing, and ANSI-colored interactive terminal rendering with pixel-perfect box alignment.

---

## 📸 System Previews

<div align="center">
  <img src="assets/screenshots/menu.png" width="48%" alt="Main Menu" />
  <img src="assets/screenshots/triage.png" width="48%" alt="Triage Priority Sorting" />
</div>

<div align="center">
  <img src="assets/screenshots/bed_grid.png" width="48%" alt="2D Ward Bed Matrix" />
  <img src="assets/screenshots/discharge.png" width="48%" alt="Discharge & Billing Receipt" />
</div>

<div align="center">
  <img src="assets/screenshots/summary.png" width="60%" alt="Hospital Analytics Dashboard" />
</div>

---

## 🏛️ System Architecture & Workflow

The system follows a modular architectural pipeline connecting emergency patient intake to triage, bed allotment, clinical care, and recursive billing:

<div align="center">
  <img src="assets/flowchart.png" width="95%" alt="Hospital Management System Architecture & Flowchart" />
</div>

---

## 📋 Key Features & C Implementation Mapping

| Module | Feature Description | Core C Syllabus Concepts |
|---|---|---|
| **1. Patient Registration** | Ingests emergency/routine patients with input validation, assigning sequential auto-incremented IDs. | `struct Patient`, `fgets`, `scanf`, `fwrite` (binary append) |
| **2. Triage Prioritization** | Sorts waiting queue placing critical emergency cases first, preserving arrival FIFO order. | Dynamic `malloc`, pointer arithmetic `*(arr + i)`, bubble sort |
| **3. Doctor Assignment** | Assigns specialized physicians across departments; enforces a hard 5-patient workload limit. | `struct Doctor[]`, in-place binary file update via `fseek` |
| **4. Bed Occupancy Grid** | Renders 3 Wards (General, ICU, Pediatric) × 10 Beds in a 2D occupancy matrix (`[O]` occupied, `[ ]` free). | 2D arrays, pointer-to-array `int (*grid)[10]` parameter passing |
| **5. Bed Allocation** | Automated discovery of first available bed in selected ward with immediate state persistence. | `fread`/`fwrite`, pointer dereferencing, multi-file synchronization |
| **6. Treatment Charges** | Records itemized medical expenses (consultation, pharmacy, daily room rate, diagnostics). | Nested structures (`struct Charges`), record updating |
| **7. Discharge & Billing** | Frees bed, writes itemized text summary (`discharge_<ID>.txt`), and calculates bill. | **Recursion** (`totalBill`), `fprintf`, file pointer manipulation |
| **8. Patient Search** | Substring and exact-match search by patient ID or full name. | `strstr`, `strcmp`, dynamic allocation with `calloc` |
| **9. Record Updating** | In-place modification of clinical diagnoses and admission status. | Binary random access file updates using `fseek(SEEK_SET)` |
| **10. Registry Table** | Formatted tabular display of all active, admitted, and discharged patient records. | Record traversal with `fread`, formatted `printf` width specifiers |
| **11. Analytics Dashboard** | Real-time census: admitted counts, bed availability ratio, and total hospital revenue. | **Recursive counting** (`countEmergency`), arithmetic aggregation |
| **99. Demo Data Ingestion** | Instant provisioning of 5 realistic emergency and routine clinical cases for demonstration. | Batch binary serialization (hidden utility) |

---

## 🎯 Syllabus Compliance (26CAH-101)

This project strictly adheres to the foundational C curriculum of Chandigarh University:
- **Language Standard:** C99 compiled with `-Wall -Wextra -Wpedantic`.
- **Allowed Standard Libraries:** `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<ctype.h>`.
- **Prohibited Constructs Respected:** Zero `typedef`, zero `union`, zero `enum`, no linked lists or node structures, no function pointers, no external database dependencies, and no platform-dependent GUI libraries.
- **Advanced Core C Techniques:**
  - **Recursion:** `totalBill(float charges[], int n)` sums itemized hospital expenses recursively; `countEmergency(struct Patient *arr, int n)` counts critical patients without iterative loops.
  - **Pointers & Pointer Arithmetic:** Pointer-to-array parameter passing `int (*grid)[BEDS_PER_WARD]`, array indexing via pointer offset `*(arr + j)`, and direct structure pointer manipulation `struct Patient *p`.
  - **Binary File Handling:** Robust binary persistence via `patients.dat` and `beds.dat` utilizing `fopen`, `fread`, `fwrite`, `fseek`, and `fclose`.

---

## 📂 Project Organization

```text
smart-hospital-management-system/
├── main.c                          # Complete single-file C99 implementation
├── README.md                       # Comprehensive project documentation
├── .gitignore                      # Git exclusion rules for binaries, data & cache
├── docs/                           # University submission deliverables
│   ├── Smart_Hospital_Management_System_Report.pdf       # 12-page academic project report
│   ├── Smart_Hospital_Management_System_Report.docx      # Editable Word manuscript
│   └── Smart_Hospital_Management_System_Presentation.pptx # 10-slide seminar presentation
├── assets/                         # Diagrams and visual media
│   ├── flowchart.png               # High-resolution architectural flowchart
│   └── screenshots/                # Terminal screenshots captured during execution
│       ├── menu.png                # Boxed main navigation menu
│       ├── register.png            # Patient intake interface
│       ├── triage.png              # Priority-sorted triage queue
│       ├── bed_grid.png            # 2D ward-bed allocation matrix
│       ├── discharge.png           # Formatted discharge receipt
│       └── summary.png             # Hospital analytics summary
└── tests/                          # Automated quality assurance suite
    └── smoke_test.py               # Sandboxed C99 compiler & runtime test
```

---

## 🛠️ Build and Execution Guide

### Prerequisites
- GCC / Clang C compiler supporting C99 standard
- Any standard POSIX terminal (macOS Terminal, Linux Bash/Zsh, or VS Code integrated terminal)

### Compilation
Compile with strict error and style warnings enabled:
```bash
gcc -std=c99 -Wall -Wextra -Wpedantic main.c -o hospital
```

### Running the System
```bash
./hospital
```

### Quick Demonstration
1. When the main menu loads, type `99` and hit **Enter** to instantly load 5 demo patients.
2. Select `2` to inspect the **Triage List** sorted with emergency patients prioritized first.
3. Select `4` to view the **Bed Grid** showing allocated beds across wards.
4. Select `7` and enter patient ID `1` to discharge the patient and view the **Recursive Discharge Receipt**.
5. Select `11` to view the aggregated **Hospital Summary Dashboard**.

---

## 🧪 Automated Testing Suite

The repository includes a fully automated smoke test suite located in `tests/smoke_test.py`. It:
- Compiles `main.c` with `-std=c99 -Wall -Wextra -Wpedantic` and verifies zero warnings.
- Spawns an isolated `tempfile.TemporaryDirectory()` sandbox.
- Tests the entire lifecycle: demo injection, registration, triage, doctor assignment, bed allocation, treatment fees, discharge calculation, search, dashboard, and clean exit.
- Verifies that zero temporary data files leak into the source repository.

To execute the test:
```bash
python3 tests/smoke_test.py
```

---

## 👨‍💻 Academic Details

| Parameter | Information |
|---|---|
| **Student Name** | Palash Behura |
| **Course** | Computer Programming (26CAH-101) |
| **Degree Program** | Bachelor of Computer Applications (BCA) |
| **Institution** | Chandigarh University, Mohali, Punjab |
| **Repository** | [smart-hospital-management-system](https://github.com/palashbehura54-source/smart-hospital-management-system) |
| **Academic Session** | 2026 – 2027 |
