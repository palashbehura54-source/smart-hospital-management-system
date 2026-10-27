/*
 * ============================================================
 *  Smart Hospital Management System with Emergency Triage
 *  Course : Computer Programming (26CAH-101)
 *  College: Chandigarh University, Mohali, Punjab
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── ANSI Color / Style Codes ───────────────────────────────── */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

/* Clear screen without system() */
#define CLEAR() printf("\033[2J\033[H")

/* ── Box geometry ───────────────────────────────────────────── */
/* BOX_W: visible character count between the two ║ border chars */
#define BOX_W 56

/* ── Constants ──────────────────────────────────────────────── */
#define DEMO_PATIENT_COUNT 30
#define MAX_DOCTORS        5
#define MAX_PER_DOC        5
#define WARDS              3
#define BEDS_PER_WARD      10
#define PATIENTS_FILE      "patients.dat"
#define BEDS_FILE          "beds.dat"

/* ── Structures ─────────────────────────────────────────────── */

/* Holds all treatment charges for one patient */
struct Charges {
    float consultation;
    float medicine;
    float roomPerDay;
    float tests;
};

/* Core patient record stored in binary file */
struct Patient {
    int   id;
    char  name[60];
    int   age;
    char  gender;       /* 'M' / 'F' / 'O' */
    char  disease[80];
    int   priority;     /* 1 = Emergency, 2 = Normal */
    int   doctorId;     /* -1 if not assigned */
    int   wardNo;       /* 0-2, -1 if no bed */
    int   bedNo;        /* 0-9, -1 if no bed */
    int   daysAdmitted;
    int   status;       /* 0 = waiting, 1 = admitted, 2 = discharged */
    struct Charges charges;
};

/* Doctor record – preloaded at startup */
struct Doctor {
    int  id;
    char name[60];
    char department[40];
    int  patientCount;
};

/* ── Globals ─────────────────────────────────────────────────── */
struct Doctor doctors[MAX_DOCTORS];
int beds[WARDS][BEDS_PER_WARD];

/* ── Box-drawing helpers ─────────────────────────────────────── */

/* Top border: ╔ followed by exactly BOX_W '═' chars then ╗ */
void boxTop(void)
{
    int i;
    printf("  " CYAN "╔");
    for (i = 0; i < BOX_W; i++) printf("═");
    printf("╗\n" RESET);
}

/* Middle divider: ╠ followed by exactly BOX_W '═' chars then ╣ */
void boxMid(void)
{
    int i;
    printf("  " CYAN "╠");
    for (i = 0; i < BOX_W; i++) printf("═");
    printf("╣\n" RESET);
}

/* Bottom border: ╚ followed by exactly BOX_W '═' chars then ╝ */
void boxBottom(void)
{
    int i;
    printf("  " CYAN "╚");
    for (i = 0; i < BOX_W; i++) printf("═");
    printf("╝\n" RESET);
}

/* Content line: ║ + color + plain text + padding-to-BOX_W + ║
   RULE: text must contain NO ANSI codes so strlen gives true width. */
void boxLine(const char *color, const char *text)
{
    int len = (int)strlen(text);
    int pad = BOX_W - len;
    int i;
    if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET "%s%s" RESET, color, text);
    for (i = 0; i < pad; i++) printf(" ");
    printf(CYAN "║\n" RESET);
}

/* Centered content line: plain text centered within BOX_W chars */
void boxLineCenter(const char *color, const char *text)
{
    int len   = (int)strlen(text);
    int left  = (BOX_W - len) / 2;
    int right = BOX_W - len - left;
    int i;
    if (left  < 0) left  = 0;
    if (right < 0) right = 0;
    printf("  " CYAN "║" RESET);
    for (i = 0; i < left;  i++) printf(" ");
    printf("%s%s" RESET, color, text);
    for (i = 0; i < right; i++) printf(" ");
    printf(CYAN "║\n" RESET);
}

/* Blank line inside a box */
void boxEmpty(void)
{
    int i;
    printf("  " CYAN "║" RESET);
    for (i = 0; i < BOX_W; i++) printf(" ");
    printf(CYAN "║\n" RESET);
}

/* Menu option line.
   numStr: "1".."11" or "0".  label: option text.
   Padding computed from PLAIN text length, never from the colored string.
   Numbers are right-aligned within a 2-char field so dots stay in column. */
void boxMenuLine(const char *numStr, const char *label,
                 const char *numColor, const char *labelColor)
{
    /* right-align single-digit numbers with a leading space */
    const char *sp = ((int)strlen(numStr) < 2) ? " " : "";
    char plain[128];
    int len, pad, i;
    sprintf(plain, "  %s%s.  %s", sp, numStr, label);
    len = (int)strlen(plain);
    pad = BOX_W - len;
    if (pad < 0) pad = 0;
    /* print the colored version — no width specifiers that touch ANSI strings */
    printf("  " CYAN "║" RESET "  %s%s%s" RESET ".  %s%s" RESET,
           numColor, sp, numStr, labelColor, label);
    for (i = 0; i < pad; i++) printf(" ");
    printf(CYAN "║\n" RESET);
}

/* ── Forward declarations ────────────────────────────────────── */
void  initDoctors(void);
void  recalculateDoctorLoads(void);
void  loadBeds(void);
void  saveBeds(void);
int   nextPatientId(void);
void  clearInputBuffer(void);
void  stripNewline(char *s);
void  pauseEnter(void);
void  printSectionHeader(const char *title);
void  printSuccess(const char *msg);
void  printError(const char *msg);
void  printWarning(const char *msg);
void  showMenu(void);
void  registerPatient(struct Patient *p);
void  triageList(void);
void  assignDoctor(void);
void  showBeds(int (*grid)[BEDS_PER_WARD]);
void  allocateBed(struct Patient *p);
void  addCharges(void);
void  dischargePatient(struct Patient *p);
void  searchPatient(void);
void  updatePatient(struct Patient *p);
void  displayAll(void);
void  hospitalSummary(void);
void  loadDemoPatients(void);
float totalBill(float charges[], int n);
int   countEmergency(struct Patient *arr, int n);

/* =============================================================
 *  RECURSIVE FUNCTION 1 – totalBill
 *  BASE CASE    : n == 0 → return 0
 *  RECURSIVE CASE: return charges[n-1] + totalBill(charges, n-1)
 * ============================================================= */
/* RECURSION USED HERE */
float totalBill(float charges[], int n)
{
    if (n == 0) return 0.0f;                             /* BASE CASE */
    return charges[n - 1] + totalBill(charges, n - 1);  /* RECURSIVE CASE */
}

/* =============================================================
 *  RECURSIVE FUNCTION 2 – countEmergency
 *  BASE CASE    : n == 0 → return 0
 *  RECURSIVE CASE: check last element, add countEmergency(arr,n-1)
 * ============================================================= */
/* RECURSION USED HERE */
int countEmergency(struct Patient *arr, int n)
{
    if (n == 0) return 0;  /* BASE CASE */
    /* POINTER USED HERE – pointer arithmetic to reach (n-1)th element */
    int isEmergency = ((arr + n - 1)->priority == 1) ? 1 : 0;
    return isEmergency + countEmergency(arr, n - 1);  /* RECURSIVE CASE */
}

/* =============================================================
 *  pauseEnter – wait for Enter key, then caller will redraw menu
 * ============================================================= */
void pauseEnter(void)
{
    int c;
    printf("\n  " YELLOW "Press Enter to continue..." RESET);
    c = getchar();
    if (c != '\n' && c != EOF) {
        while ((c = getchar()) != '\n' && c != EOF);
    }
}

/* =============================================================
 *  printSectionHeader – consistent-width section banner
 * ============================================================= */
void printSectionHeader(const char *title)
{
    printf("\n");
    boxTop();
    boxLineCenter(BOLD WHITE, title);
    boxBottom();
    printf("\n");
}

/* Feedback messages: all plain ASCII tags to avoid glyph-width issues */
void printSuccess(const char *msg)
{
    printf("  " GREEN BOLD "[OK] %s" RESET "\n", msg);
}

void printError(const char *msg)
{
    printf("  " RED BOLD " [X] %s" RESET "\n", msg);
}

/* ⚠ replaced with plain ASCII [ ! ] – double-width glyph breaks alignment */
void printWarning(const char *msg)
{
    printf("  " YELLOW BOLD "[ ! ] %s" RESET "\n", msg);
}

/* =============================================================
 *  showMenu – main menu rendered with consistent-width box helpers
 * ============================================================= */
void showMenu(void)
{
    CLEAR();
    boxTop();
    boxLineCenter(BOLD CYAN,  "SMART HOSPITAL MANAGEMENT SYSTEM");
    boxLineCenter(WHITE,      "Chandigarh University, Mohali");
    boxLineCenter(WHITE,      "Course: 26CAH-101 | C Programming");
    boxMid();
    boxEmpty();
    boxMenuLine("1",  "Register Patient",              YELLOW, WHITE);
    boxMenuLine("2",  "Triage List  (Emergency First)", YELLOW, WHITE);
    boxMenuLine("3",  "Assign Doctor to Patient",      YELLOW, WHITE);
    boxMenuLine("4",  "Show Bed Grid",                 YELLOW, WHITE);
    boxMenuLine("5",  "Allocate Bed to Patient",       YELLOW, WHITE);
    boxMenuLine("6",  "Add Treatment Charges",         YELLOW, WHITE);
    boxMenuLine("7",  "Discharge Patient",             YELLOW, WHITE);
    boxMenuLine("8",  "Search Patient",                YELLOW, WHITE);
    boxMenuLine("9",  "Update Patient Details",        YELLOW, WHITE);
    boxMenuLine("10", "Display All Patients",          YELLOW, WHITE);
    boxMenuLine("11", "Hospital Summary",              YELLOW, WHITE);
    boxMenuLine("0",  "Exit",                          RED,    RED);
    boxEmpty();
    boxBottom();
    printf("  " BOLD WHITE "Your choice: " RESET);
}

/* =============================================================
 *  initDoctors – Preload 5 doctors at program start
 * ============================================================= */
void initDoctors(void)
{
    doctors[0].id = 1;
    strcpy(doctors[0].name, "Dr. Priya Sharma");
    strcpy(doctors[0].department, "Emergency");
    doctors[0].patientCount = 0;

    doctors[1].id = 2;
    strcpy(doctors[1].name, "Dr. Rohit Mehta");
    strcpy(doctors[1].department, "General Medicine");
    doctors[1].patientCount = 0;

    doctors[2].id = 3;
    strcpy(doctors[2].name, "Dr. Anjali Singh");
    strcpy(doctors[2].department, "Cardiology");
    doctors[2].patientCount = 0;

    doctors[3].id = 4;
    strcpy(doctors[3].name, "Dr. Karan Verma");
    strcpy(doctors[3].department, "Orthopedics");
    doctors[3].patientCount = 0;

    doctors[4].id = 5;
    strcpy(doctors[4].name, "Dr. Nisha Patel");
    strcpy(doctors[4].department, "Pediatrics");
    doctors[4].patientCount = 0;
}

/* =============================================================
 *  recalculateDoctorLoads – Tally active patients per doctor
 * ============================================================= */
void recalculateDoctorLoads(void)
{
    int i;
    for (i = 0; i < MAX_DOCTORS; i++)
        doctors[i].patientCount = 0;

    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) return;

    struct Patient p;
    while (fread(&p, sizeof(struct Patient), 1, f) == 1) {
        if (p.status != 2 && p.doctorId >= 1 && p.doctorId <= MAX_DOCTORS) {
            doctors[p.doctorId - 1].patientCount++;
        }
    }
    fclose(f);
}

/* =============================================================
 *  loadBeds – Load bed occupancy grid from beds.dat
 * ============================================================= */
void loadBeds(void)
{
    FILE *f = fopen(BEDS_FILE, "rb");
    if (f == NULL) {
        int w, b;
        for (w = 0; w < WARDS; w++)
            for (b = 0; b < BEDS_PER_WARD; b++)
                beds[w][b] = 0;
        return;
    }
    fread(beds, sizeof(beds), 1, f);
    fclose(f);
}

/* =============================================================
 *  saveBeds – Save bed occupancy grid to beds.dat
 * ============================================================= */
void saveBeds(void)
{
    FILE *f = fopen(BEDS_FILE, "wb");
    if (f == NULL) { printError("Cannot save bed data."); return; }
    fwrite(beds, sizeof(beds), 1, f);
    fclose(f);
}

/* =============================================================
 *  nextPatientId – Count existing records, return next ID
 * ============================================================= */
int nextPatientId(void)
{
    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) return 1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return (int)(size / sizeof(struct Patient)) + 1;
}

/* =============================================================
 *  clearInputBuffer – Discard leftover characters after scanf
 * ============================================================= */
void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* =============================================================
 *  stripNewline – Remove trailing '\n' left by fgets
 * ============================================================= */
void stripNewline(char *s)
{
    int len = (int)strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

/* =============================================================
 *  registerPatient – Fill patient struct and save to patients.dat
 *  POINTER USED HERE – p is passed as struct Patient *
 * ============================================================= */
void registerPatient(struct Patient *p) /* POINTER USED HERE */
{
    p->id = nextPatientId();

    printf("  " CYAN "Name        : " RESET);
    fgets(p->name, sizeof(p->name), stdin);
    stripNewline(p->name);

    /* Validate age */
    do {
        printf("  " CYAN "Age (0-120) : " RESET);
        scanf("%d", &p->age);
        clearInputBuffer();
        if (p->age < 0 || p->age > 120)
            printError("Invalid age. Try again.");
    } while (p->age < 0 || p->age > 120);

    /* Validate gender */
    do {
        printf("  " CYAN "Gender (M/F/O): " RESET);
        scanf(" %c", &p->gender);
        clearInputBuffer();
        p->gender = (char)toupper((unsigned char)p->gender);
        if (p->gender != 'M' && p->gender != 'F' && p->gender != 'O')
            printError("Enter M, F, or O.");
    } while (p->gender != 'M' && p->gender != 'F' && p->gender != 'O');

    printf("  " CYAN "Disease     : " RESET);
    fgets(p->disease, sizeof(p->disease), stdin);
    stripNewline(p->disease);

    /* Validate priority */
    do {
        printf("  " CYAN "Priority (1=Emergency, 2=Normal): " RESET);
        scanf("%d", &p->priority);
        clearInputBuffer();
        if (p->priority != 1 && p->priority != 2)
            printError("Enter 1 or 2.");
    } while (p->priority != 1 && p->priority != 2);

    /* Defaults */
    p->doctorId             = -1;
    p->wardNo               = -1;
    p->bedNo                = -1;
    p->daysAdmitted         = 0;
    p->status               = 0;
    p->charges.consultation = 0.0f;
    p->charges.medicine     = 0.0f;
    p->charges.roomPerDay   = 0.0f;
    p->charges.tests        = 0.0f;

    /* Append to binary file */
    FILE *f = fopen(PATIENTS_FILE, "ab");
    if (f == NULL) { printError("Cannot open patients file."); return; }
    fwrite(p, sizeof(struct Patient), 1, f);
    fclose(f);

    printf("\n");
    {
        char buf[80];
        sprintf(buf, "Patient registered! ID = %d", p->id);
        printSuccess(buf);
    }
}

/* =============================================================
 *  triageList – Load all patients, bubble-sort by priority,
 *               display as aligned colored table.
 *  POINTER USED HERE – pointer arithmetic on dynamic array.
 * ============================================================= */
void triageList(void)
{
    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) { printError("No patient records found."); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    int total = (int)(size / sizeof(struct Patient));
    if (total == 0) { fclose(f); printWarning("No patients registered."); return; }

    /* POINTER USED HERE – dynamic array via malloc */
    struct Patient *arr = (struct Patient *)malloc(total * sizeof(struct Patient));
    if (arr == NULL) { fclose(f); printError("malloc failed."); return; }
    fread(arr, sizeof(struct Patient), total, f);
    fclose(f);

    /* Bubble sort: priority 1 (Emergency) before 2 (Normal) */
    int i, j;
    struct Patient tmp;
    for (i = 0; i < total - 1; i++) {
        for (j = 0; j < total - i - 1; j++) {
            /* POINTER USED HERE – pointer arithmetic */
            if ((arr + j)->priority > (arr + j + 1)->priority) {
                tmp            = *(arr + j);
                *(arr + j)     = *(arr + j + 1);
                *(arr + j + 1) = tmp;
            }
        }
    }

    /* Column widths (visible): ID=4, Name=20, Age=3, Disease=16,
       Priority=11, Status=10.  Each cell has one space of padding on each side. */
    printf(CYAN
       "  ┌──────┬──────────────────────┬─────┬──────────────────┬─────────────┬────────────┐\n"
       RESET);
    printf(CYAN "  │" RESET BOLD WHITE " %-4s " RESET
           CYAN "│" RESET BOLD WHITE " %-20s " RESET
           CYAN "│" RESET BOLD WHITE " %-3s " RESET
           CYAN "│" RESET BOLD WHITE " %-16s " RESET
           CYAN "│" RESET BOLD WHITE " %-11s " RESET
           CYAN "│" RESET BOLD WHITE " %-10s " RESET
           CYAN "│\n" RESET,
           "ID", "Name", "Age", "Disease", "Priority", "Status");
    printf(CYAN
       "  ├──────┼──────────────────────┼─────┼──────────────────┼─────────────┼────────────┤\n"
       RESET);

    int printed = 0;
    for (i = 0; i < total; i++) {
        /* POINTER USED HERE – pointer arithmetic to walk array */
        struct Patient *cur = (arr + i);
        if (cur->status == 2) continue;

        const char *rowColor = (cur->priority == 1) ? RED : GREEN;
        const char *prioTag  = (cur->priority == 1) ? "[EMERGENCY]" : "[NORMAL]   ";
        const char *statStr  = (cur->status  == 0)  ? "Waiting"
                             : (cur->status  == 1)  ? "Admitted"
                                                    : "Discharged";
        /* Color codes are placed OUTSIDE %-Ns specifiers so widths are unaffected */
        printf(CYAN "  │" RESET " %s%-4d" RESET " "
               CYAN "│" RESET " %s%-20s" RESET " "
               CYAN "│" RESET " %s%-3d" RESET " "
               CYAN "│" RESET " %s%-16s" RESET " "
               CYAN "│" RESET " %s%-11s" RESET " "
               CYAN "│" RESET " %s%-10s" RESET " "
               CYAN "│\n" RESET,
               rowColor, cur->id,
               rowColor, cur->name,
               rowColor, cur->age,
               rowColor, cur->disease,
               rowColor, prioTag,
               rowColor, statStr);
        printed++;
    }

    if (printed == 0)
        printf(CYAN "  │" RESET "%-84s" CYAN "│\n" RESET,
               "  No active patients.");

    printf(CYAN
       "  └──────┴──────────────────────┴─────┴──────────────────┴─────────────┴────────────┘\n"
       RESET);

    free(arr);  /* always free dynamic memory */
}

/* =============================================================
 *  assignDoctor – Link a patient to a doctor (max 5 per doctor)
 * ============================================================= */
void assignDoctor(void)
{
    int pid, did, i;
    printf("  " CYAN "Patient ID: " RESET);
    scanf("%d", &pid); clearInputBuffer();

    printf("\n  " BOLD WHITE "Available Doctors:\n" RESET);
    printf(CYAN "  %-4s %-25s %-18s %s\n" RESET,
           "ID", "Name", "Department", "Load");
    printf(CYAN "  %-60s\n" RESET,
           "────────────────────────────────────────────────────────────");
    for (i = 0; i < MAX_DOCTORS; i++) {
        const char *lc = (doctors[i].patientCount >= MAX_PER_DOC) ? RED : GREEN;
        printf("  " YELLOW "%-4d" RESET
               WHITE "%-25s " RESET
               MAGENTA "%-18s " RESET
               "%s%d/%d\n" RESET,
               doctors[i].id, doctors[i].name, doctors[i].department,
               lc, doctors[i].patientCount, MAX_PER_DOC);
    }

    printf("\n  " CYAN "Doctor ID : " RESET);
    scanf("%d", &did); clearInputBuffer();

    if (did < 1 || did > MAX_DOCTORS) {
        printError("Invalid doctor ID."); return;
    }
    if (doctors[did - 1].patientCount >= MAX_PER_DOC) {
        printError("Doctor is full (max 5 patients)."); return;
    }

    FILE *f = fopen(PATIENTS_FILE, "r+b");
    if (f == NULL) { printError("No patient file found."); return; }

    struct Patient p;
    long pos = 0;
    int found = 0;
    while (fread(&p, sizeof(struct Patient), 1, f) == 1) {
        if (p.id == pid && p.status != 2) {
            p.doctorId = did;
            fseek(f, pos, SEEK_SET);  /* POINTER USED HERE – in-place update */
            fwrite(&p, sizeof(struct Patient), 1, f);
            found = 1;
            doctors[did - 1].patientCount++;
            break;
        }
        pos += (long)sizeof(struct Patient);
    }
    fclose(f);

    if (!found) printError("Patient not found or already discharged.");
    else        printSuccess("Doctor assigned successfully.");
}

/* =============================================================
 *  showBeds – Print the ward-bed grid with color coding
 *  POINTER USED HERE – grid passed as pointer to array of 10 ints
 * ============================================================= */
void showBeds(int (*grid)[BEDS_PER_WARD]) /* POINTER USED HERE */
{
    const char *wardNames[3] = {
        "Ward A  General   ",
        "Ward B  ICU       ",
        "Ward C  Pediatric "
    };
    int w, b, totalFree = 0, totalOcc = 0;

    printf("\n");
    for (w = 0; w < WARDS; w++) {
        printf("  " CYAN BOLD "%-20s" RESET "  ", wardNames[w]);
        for (b = 0; b < BEDS_PER_WARD; b++) {
            /* POINTER USED HERE – pointer arithmetic on 2D via pointer-to-array */
            if (*(*(grid + w) + b) == 1) {
                printf(RED "[O]" RESET " ");
                totalOcc++;
            } else {
                printf(GREEN "[ ]" RESET " ");
                totalFree++;
            }
        }
        printf("\n");
    }
    printf("\n  " WHITE "Beds 1-10 above.  "
           RED   "[O] = Occupied  " RESET
           GREEN "[ ] = Free\n" RESET);
    printf("  " YELLOW "Total: " WHITE "%d occupied" RESET
           ", " GREEN "%d free" RESET
           " of " WHITE "%d" RESET " beds.\n",
           totalOcc, totalFree, WARDS * BEDS_PER_WARD);
}

/* =============================================================
 *  allocateBed – Assign first free bed in chosen ward to patient
 *  POINTER USED HERE – p is struct Patient *
 * ============================================================= */
void allocateBed(struct Patient *p) /* POINTER USED HERE */
{
    FILE *f = fopen(PATIENTS_FILE, "r+b");
    if (f == NULL) { printError("Cannot open patient file."); return; }

    struct Patient rec;
    long pos = 0;
    int foundPatient = 0;
    while (fread(&rec, sizeof(struct Patient), 1, f) == 1) {
        if (rec.id == p->id) {
            foundPatient = 1;
            break;
        }
        pos += (long)sizeof(struct Patient);
    }

    if (!foundPatient) {
        fclose(f);
        printError("Patient ID not found.");
        return;
    }
    if (rec.status == 1) {
        fclose(f);
        printError("Patient is already admitted with an allocated bed.");
        return;
    }
    if (rec.status == 2) {
        fclose(f);
        printError("Patient is already discharged.");
        return;
    }

    int wChoice;
    showBeds(beds);
    printf("\n  " CYAN "Choose ward (1=General, 2=ICU, 3=Pediatric): " RESET);
    scanf("%d", &wChoice); clearInputBuffer();
    if (wChoice < 1 || wChoice > 3) {
        fclose(f);
        printError("Invalid ward number.");
        return;
    }
    int w = wChoice - 1;

    int b, foundBed = -1;
    for (b = 0; b < BEDS_PER_WARD; b++) {
        if (beds[w][b] == 0) { foundBed = b; break; }
    }
    if (foundBed == -1) {
        fclose(f);
        printError("Ward is full. No free beds.");
        return;
    }

    beds[w][foundBed] = 1;
    saveBeds();

    rec.wardNo = w;
    rec.bedNo  = foundBed;
    rec.status = 1;
    fseek(f, pos, SEEK_SET);
    fwrite(&rec, sizeof(struct Patient), 1, f);
    fclose(f);

    p->wardNo = w;
    p->bedNo  = foundBed;
    p->status = 1;

    {
        char buf[60];
        sprintf(buf, "Bed allocated: Ward %d, Bed %d", w + 1, foundBed + 1);
        printSuccess(buf);
    }
}

/* =============================================================
 *  addCharges – Add / update treatment charges for a patient
 * ============================================================= */
void addCharges(void)
{
    int pid;
    printf("  " CYAN "Patient ID: " RESET);
    scanf("%d", &pid); clearInputBuffer();

    FILE *f = fopen(PATIENTS_FILE, "r+b");
    if (f == NULL) { printError("No patient file."); return; }

    struct Patient p;
    long pos = 0;
    int found = 0;
    while (fread(&p, sizeof(struct Patient), 1, f) == 1) {
        if (p.id == pid && p.status != 2) {
            float c, m, r, t;
            printf("  " CYAN "Consultation fee  " YELLOW "(current %.2f)" CYAN ": " RESET,
                   p.charges.consultation);
            scanf("%f", &c); clearInputBuffer();
            printf("  " CYAN "Medicine cost     " YELLOW "(current %.2f)" CYAN ": " RESET,
                   p.charges.medicine);
            scanf("%f", &m); clearInputBuffer();
            printf("  " CYAN "Room per day      " YELLOW "(current %.2f)" CYAN ": " RESET,
                   p.charges.roomPerDay);
            scanf("%f", &r); clearInputBuffer();
            printf("  " CYAN "Test charges      " YELLOW "(current %.2f)" CYAN ": " RESET,
                   p.charges.tests);
            scanf("%f", &t); clearInputBuffer();
            printf("  " CYAN "Days admitted     " YELLOW "(current %d)" CYAN ": " RESET,
                   p.daysAdmitted);
            scanf("%d", &p.daysAdmitted); clearInputBuffer();

            if (c < 0 || m < 0 || r < 0 || t < 0 || p.daysAdmitted < 0) {
                printError("Charges and days cannot be negative.");
                fclose(f); return;
            }
            p.charges.consultation = c;
            p.charges.medicine     = m;
            p.charges.roomPerDay   = r;
            p.charges.tests        = t;

            fseek(f, pos, SEEK_SET);
            fwrite(&p, sizeof(struct Patient), 1, f);
            found = 1;
            break;
        }
        pos += (long)sizeof(struct Patient);
    }
    fclose(f);
    if (!found) printError("Patient not found.");
    else        printSuccess("Charges updated successfully.");
}

/* =============================================================
 *  dischargePatient – Free bed, compute bill (recursive),
 *                     print receipt box, write .txt summary.
 *  POINTER USED HERE – p is struct Patient *
 * ============================================================= */
void dischargePatient(struct Patient *p) /* POINTER USED HERE */
{
    int pid;
    printf("  " CYAN "Patient ID to discharge: " RESET);
    scanf("%d", &pid); clearInputBuffer();

    FILE *f = fopen(PATIENTS_FILE, "r+b");
    if (f == NULL) { printError("No patient file."); return; }

    struct Patient rec;
    long pos = 0;
    int found = 0;
    while (fread(&rec, sizeof(struct Patient), 1, f) == 1) {
        if (rec.id == pid && rec.status == 1) {
            /* Free the bed */
            if (rec.wardNo >= 0 && rec.bedNo >= 0) {
                beds[rec.wardNo][rec.bedNo] = 0;
                saveBeds();
                rec.wardNo = -1;
                rec.bedNo  = -1;
            }

            /* Build charges array for recursive totalBill */
            float ch[4];
            ch[0] = rec.charges.consultation;
            ch[1] = rec.charges.medicine;
            ch[2] = rec.charges.roomPerDay * (float)rec.daysAdmitted;
            ch[3] = rec.charges.tests;

            /* RECURSION USED HERE – totalBill sums the array recursively */
            float bill = totalBill(ch, 4);

            rec.status = 2;  /* discharged */
            fseek(f, pos, SEEK_SET);
            fwrite(&rec, sizeof(struct Patient), 1, f);
            *p = rec;
            found = 1;

            if (rec.doctorId >= 1 && rec.doctorId <= MAX_DOCTORS)
                doctors[rec.doctorId - 1].patientCount--;

            /* ── Discharge receipt box ─────────────────────────
               All padding computed from PLAIN text length via sprintf.
               ─────────────────────────────────────────────────── */
            char tmp[160];
            int  pad, i;

            printf("\n");
            boxTop();
            boxLineCenter(BOLD WHITE, "DISCHARGE RECEIPT");
            boxMid();

            /* Patient ID */
            sprintf(tmp, "  Patient ID   : %d", rec.id);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Patient ID   : " YELLOW "%d" RESET, rec.id);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Name */
            sprintf(tmp, "  Name         : %s", rec.name);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Name         : " YELLOW "%s" RESET, rec.name);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Disease */
            sprintf(tmp, "  Disease      : %s", rec.disease);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Disease      : " YELLOW "%s" RESET, rec.disease);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Days admitted */
            sprintf(tmp, "  Days Admitted: %d", rec.daysAdmitted);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Days Admitted: " YELLOW "%d" RESET, rec.daysAdmitted);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            boxMid();

            /* Consultation */
            sprintf(tmp, "  Consultation : Rs. %.2f", ch[0]);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Consultation : Rs. " WHITE "%.2f" RESET, ch[0]);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Medicine */
            sprintf(tmp, "  Medicine     : Rs. %.2f", ch[1]);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Medicine     : Rs. " WHITE "%.2f" RESET, ch[1]);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Room charges */
            sprintf(tmp, "  Room charges : Rs. %.2f", ch[2]);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Room charges : Rs. " WHITE "%.2f" RESET, ch[2]);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            /* Tests */
            sprintf(tmp, "  Tests        : Rs. %.2f", ch[3]);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET WHITE "  Tests        : Rs. " WHITE "%.2f" RESET, ch[3]);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            boxMid();

            /* TOTAL BILL – bold green */
            sprintf(tmp, "  TOTAL BILL   : Rs. %.2f", bill);
            pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
            printf("  " CYAN "║" RESET GREEN BOLD "  TOTAL BILL   : Rs. %.2f" RESET, bill);
            for (i = 0; i < pad; i++) printf(" ");
            printf(CYAN "║\n" RESET);

            boxBottom();

            /* Write plain-text discharge summary file */
            {
                char fname[40];
                sprintf(fname, "discharge_%d.txt", pid);
                FILE *df = fopen(fname, "w");
                if (df) {
                    fprintf(df, "===== DISCHARGE SUMMARY =====\n");
                    fprintf(df, "Patient ID   : %d\n", rec.id);
                    fprintf(df, "Name         : %s\n", rec.name);
                    fprintf(df, "Age/Gender   : %d / %c\n", rec.age, rec.gender);
                    fprintf(df, "Disease      : %s\n", rec.disease);
                    fprintf(df, "Priority     : %s\n",
                            rec.priority == 1 ? "Emergency" : "Normal");
                    fprintf(df, "Days Admitted: %d\n", rec.daysAdmitted);
                    fprintf(df, "------------------------------\n");
                    fprintf(df, "Consultation : Rs. %.2f\n", ch[0]);
                    fprintf(df, "Medicine     : Rs. %.2f\n", ch[1]);
                    fprintf(df, "Room Charges : Rs. %.2f (%d days x %.2f)\n",
                            ch[2], rec.daysAdmitted, rec.charges.roomPerDay);
                    fprintf(df, "Tests        : Rs. %.2f\n", ch[3]);
                    fprintf(df, "------------------------------\n");
                    fprintf(df, "TOTAL BILL   : Rs. %.2f\n", bill);
                    fprintf(df, "==============================\n");
                    fclose(df);
                }
                {
                    char msgbuf[60];
                    sprintf(msgbuf, "Summary saved to %s", fname);
                    printSuccess(msgbuf);
                }
            }
            break;
        }
        pos += (long)sizeof(struct Patient);
    }
    fclose(f);
    if (!found) printError("Patient not found or not admitted.");
}

/* =============================================================
 *  searchPatient – Find by ID or by name substring (strstr/strcmp)
 *  POINTER USED HERE – pointer arithmetic on dynamic array
 * ============================================================= */
void searchPatient(void)
{
    int choice;
    printf("  " CYAN "Search by: " YELLOW "1" WHITE ".ID   "
                      YELLOW "2" WHITE ".Name  > " RESET);
    scanf("%d", &choice); clearInputBuffer();

    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) { printError("No records found."); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    int total = (int)(size / sizeof(struct Patient));
    if (total == 0) { fclose(f); printWarning("No patients on record."); return; }

    /* POINTER USED HERE – dynamic allocation */
    struct Patient *arr = (struct Patient *)malloc(total * sizeof(struct Patient));
    if (arr == NULL) { fclose(f); printError("malloc failed."); return; }
    fread(arr, sizeof(struct Patient), total, f);
    fclose(f);

    int i, found = 0;
    /* Table header */
    printf(CYAN "\n  %-6s %-22s %-5s %-18s %-13s %-10s\n" RESET,
           "ID", "Name", "Age", "Disease", "Priority", "Status");
    printf(CYAN "  %-74s\n" RESET,
           "──────────────────────────────────────────────────────────────────────────");

    if (choice == 1) {
        int sid;
        printf("  " CYAN "Enter Patient ID: " RESET);
        scanf("%d", &sid); clearInputBuffer();
        for (i = 0; i < total; i++) {
            /* POINTER USED HERE – pointer arithmetic */
            struct Patient *cur = (arr + i);
            if (cur->id == sid) {
                const char *col = (cur->priority == 1) ? RED : GREEN;
                const char *stat = (cur->status == 0) ? "Waiting"
                                 : (cur->status == 1) ? "Admitted" : "Discharged";
                printf("  %s%-6d" RESET " %-22s %-5d %-18s %s%-13s" RESET " %-10s\n",
                       col, cur->id, cur->name, cur->age, cur->disease,
                       col, cur->priority == 1 ? "[EMERGENCY]" : "[NORMAL]",
                       stat);
                found = 1;
            }
        }
    } else if (choice == 2) {
        char keyword[60];
        printf("  " CYAN "Enter name (or part): " RESET);
        fgets(keyword, sizeof(keyword), stdin);
        stripNewline(keyword);
        for (i = 0; i < total; i++) {
            /* POINTER USED HERE */
            struct Patient *cur = (arr + i);
            if (strstr(cur->name, keyword) != NULL) {  /* strstr substring match */
                const char *col = (cur->priority == 1) ? RED : GREEN;
                const char *stat = (cur->status == 0) ? "Waiting"
                                 : (cur->status == 1) ? "Admitted" : "Discharged";
                printf("  %s%-6d" RESET " %-22s %-5d %-18s %s%-13s" RESET " %-10s\n",
                       col, cur->id, cur->name, cur->age, cur->disease,
                       col, cur->priority == 1 ? "[EMERGENCY]" : "[NORMAL]",
                       stat);
                found = 1;
            }
        }
    } else {
        printError("Invalid choice.");
    }

    if (!found) printWarning("No matching patient found.");
    free(arr);
}

/* =============================================================
 *  updatePatient – Edit name, disease, priority, days admitted
 *  POINTER USED HERE – p is struct Patient *
 * ============================================================= */
void updatePatient(struct Patient *p) /* POINTER USED HERE */
{
    int pid;
    printf("  " CYAN "Patient ID to update: " RESET);
    scanf("%d", &pid); clearInputBuffer();

    FILE *f = fopen(PATIENTS_FILE, "r+b");
    if (f == NULL) { printError("File not found."); return; }

    struct Patient rec;
    long pos = 0;
    int found = 0;
    while (fread(&rec, sizeof(struct Patient), 1, f) == 1) {
        if (rec.id == pid && rec.status != 2) {
            char tmp[60];

            printf("  " YELLOW "Current Name   : " WHITE "%s\n" RESET, rec.name);
            printf("  " CYAN "New Name (Enter to keep): " RESET);
            fgets(tmp, sizeof(tmp), stdin);
            stripNewline(tmp);
            if (strlen(tmp) > 0) strcpy(rec.name, tmp);

            printf("  " YELLOW "Current Disease: " WHITE "%s\n" RESET, rec.disease);
            printf("  " CYAN "New Disease (Enter to keep): " RESET);
            fgets(tmp, sizeof(tmp), stdin);
            stripNewline(tmp);
            if (strlen(tmp) > 0) strcpy(rec.disease, tmp);

            int newPrio;
            printf("  " YELLOW "Current Priority: " WHITE "%d\n" RESET, rec.priority);
            printf("  " CYAN "New Priority (1/2, 0 to keep): " RESET);
            scanf("%d", &newPrio); clearInputBuffer();
            if (newPrio == 1 || newPrio == 2) rec.priority = newPrio;

            int newDays;
            printf("  " YELLOW "Current Days: " WHITE "%d\n" RESET, rec.daysAdmitted);
            printf("  " CYAN "New Days (-1 to keep): " RESET);
            scanf("%d", &newDays); clearInputBuffer();
            if (newDays >= 0) rec.daysAdmitted = newDays;

            fseek(f, pos, SEEK_SET);
            fwrite(&rec, sizeof(struct Patient), 1, f);
            *p = rec;
            found = 1;
            break;
        }
        pos += (long)sizeof(struct Patient);
    }
    fclose(f);
    if (!found) printError("Patient not found.");
    else        printSuccess("Patient record updated successfully.");
}

/* =============================================================
 *  displayAll – Show all patients in a colored table
 * ============================================================= */
void displayAll(void)
{
    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) { printError("No records found."); return; }

    printf(CYAN "  %-4s %-20s %-4s %-3s %-14s %-5s %-11s %-5s %-4s\n" RESET,
           "ID", "Name", "Age", "G", "Disease", "Prio", "Status", "Ward", "Bed");
    printf(CYAN "  %-72s\n" RESET,
           "────────────────────────────────────────────────────────────────────────");

    struct Patient p;
    int count = 0;
    while (fread(&p, sizeof(struct Patient), 1, f) == 1) {
        const char *col  = (p.priority == 1) ? RED : GREEN;
        const char *stat = (p.status == 0) ? "Waiting"
                         : (p.status == 1) ? "Admitted" : "Discharged";
        char wardStr[8], bedStr[8];
        if (p.wardNo >= 0 && p.bedNo >= 0) {
            sprintf(wardStr, "%d", p.wardNo + 1);
            sprintf(bedStr, "%d", p.bedNo + 1);
        } else {
            strcpy(wardStr, "-");
            strcpy(bedStr, "-");
        }
        printf("  %s%-4d" RESET " %-20s %-4d %-3c %-14s "
               "%s%-5d" RESET " %-11s %-5s %-4s\n",
               col, p.id, p.name, p.age, p.gender, p.disease,
               col, p.priority, stat, wardStr, bedStr);
        count++;
    }
    fclose(f);
    if (count == 0) printWarning("No patients on record.");
    else printf("\n  " WHITE "Total records: " YELLOW "%d\n" RESET, count);
}

/* =============================================================
 *  hospitalSummary – Boxed dashboard; uses recursive helpers.
 *  POINTER USED HERE – dynamic array via calloc + pointer arithmetic
 * ============================================================= */
void hospitalSummary(void)
{
    FILE *f = fopen(PATIENTS_FILE, "rb");
    if (f == NULL) { printError("No records."); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    int total = (int)(size / sizeof(struct Patient));
    if (total == 0) { fclose(f); printWarning("No patients."); return; }

    /* POINTER USED HERE – dynamic array */
    struct Patient *arr = (struct Patient *)calloc(total, sizeof(struct Patient));
    if (arr == NULL) { fclose(f); printError("calloc failed."); return; }
    fread(arr, sizeof(struct Patient), total, f);
    fclose(f);

    /* RECURSION USED HERE – count emergency patients recursively */
    int emgCount = countEmergency(arr, total);

    int freeBeds = 0, admittedCount = 0, i, w, b;
    float revenue = 0.0f;
    for (w = 0; w < WARDS; w++)
        for (b = 0; b < BEDS_PER_WARD; b++)
            if (beds[w][b] == 0) freeBeds++;

    for (i = 0; i < total; i++) {
        /* POINTER USED HERE – pointer arithmetic */
        struct Patient *cur = (arr + i);
        if (cur->status == 2) {
            float ch[4];
            ch[0] = cur->charges.consultation;
            ch[1] = cur->charges.medicine;
            ch[2] = cur->charges.roomPerDay * (float)cur->daysAdmitted;
            ch[3] = cur->charges.tests;
            revenue += totalBill(ch, 4);  /* RECURSION USED HERE */
        }
        if (cur->status == 1) admittedCount++;
    }

    /* ── Dashboard box ─────────────────────────────────────────
       Plain text built with sprintf for width; color applied after.
       ─────────────────────────────────────────────────────────── */
    char tmp[128];
    int  pad, j;

    printf("\n");
    boxTop();
    boxLineCenter(BOLD WHITE, "HOSPITAL DASHBOARD");
    boxMid();

    /* Total Patients Registered */
    sprintf(tmp, "  Total Patients Registered : %d", total);
    pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET WHITE "  Total Patients Registered : " YELLOW "%d" RESET, total);
    for (j = 0; j < pad; j++) printf(" ");
    printf(CYAN "║\n" RESET);

    /* Currently Admitted */
    sprintf(tmp, "  Currently Admitted        : %d", admittedCount);
    pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET WHITE "  Currently Admitted        : " GREEN "%d" RESET, admittedCount);
    for (j = 0; j < pad; j++) printf(" ");
    printf(CYAN "║\n" RESET);

    /* Emergency Patients */
    sprintf(tmp, "  Emergency Patients        : %d", emgCount);
    pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET WHITE "  Emergency Patients        : " RED "%d" RESET, emgCount);
    for (j = 0; j < pad; j++) printf(" ");
    printf(CYAN "║\n" RESET);

    /* Free Beds */
    sprintf(tmp, "  Free Beds (all wards)     : %d / %d",
            freeBeds, WARDS * BEDS_PER_WARD);
    pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET WHITE "  Free Beds (all wards)     : "
           GREEN "%d" WHITE " / " YELLOW "%d" RESET,
           freeBeds, WARDS * BEDS_PER_WARD);
    for (j = 0; j < pad; j++) printf(" ");
    printf(CYAN "║\n" RESET);

    /* Total Revenue */
    sprintf(tmp, "  Total Revenue (discharged): Rs. %.2f", revenue);
    pad = BOX_W - (int)strlen(tmp); if (pad < 0) pad = 0;
    printf("  " CYAN "║" RESET WHITE "  Total Revenue (discharged): "
           GREEN BOLD "Rs. %.2f" RESET, revenue);
    for (j = 0; j < pad; j++) printf(" ");
    printf(CYAN "║\n" RESET);

    boxBottom();

    free(arr);
}

/* =============================================================
 *  loadDemoPatients – Hidden option 99; loads 30 demo patients
 * ============================================================= */
void loadDemoPatients(void)
{
    printWarning("This will reset all current patient records and bed allocations!");
    printf("  " YELLOW "Are you sure you want to load 30 demo patients? (y/n): " RESET);
    char ans[16];
    if (fgets(ans, sizeof(ans), stdin) == NULL) return;
    if (ans[0] != 'y' && ans[0] != 'Y') {
        printWarning("Demo data loading cancelled. Current records preserved.");
        return;
    }

    remove(PATIENTS_FILE);

    int w, b, d;
    for (w = 0; w < WARDS; w++)
        for (b = 0; b < BEDS_PER_WARD; b++)
            beds[w][b] = 0;

    for (d = 0; d < MAX_DOCTORS; d++)
        doctors[d].patientCount = 0;

    const char *names[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        "Amit Sharma",       "Sunita Rao",        "Vikram Malhotra",
        "Pooja Verma",       "Harpreet Singh",    "Rajesh Khanna",
        "Meera Nambiar",     "Gurdeep Dhillon",   "Kavita Saxena",
        "Anil Deshmukh",     "Aarav Gupta",       "Diya Sengupta",
        "Kabir Joshi",       "Ananya Nair",       "Vihaan Chadha",
        /* Waiting (10) */
        "Rohan Sethi",       "Siddharth Bhatia",  "Deepak Chopra",
        "Fatima Sheikh",     "Neha Agarwal",      "Manish Tiwari",
        "Jaspreet Kaur",     "Sanjay Kulkarni",   "Ritu Choudhary",
        "Kiran Patel",
        /* Discharged (5) */
        "Prakash Goswami",   "Kalyani Iyer",      "Tarun Chatterjee",
        "Shalini Mishra",    "Rameshwar Prasad"
    };

    const char *diseases[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        "Acute Myocardial Infarction", "Type 2 Diabetes Flare",    "Hypertensive Crisis",
        "Severe Gastroenteritis",      "Compound Tibia Fracture",   "Cardiogenic Shock",
        "Acute Respiratory Distress",  "Septicemia",                "Acute Ischemic Stroke",
        "Post-Cardiac Bypass Care",    "Severe Bronchopneumonia",   "Dengue with Shock",
        "Acute Asthma Exacerbation",   "Rotavirus Dehydration",     "Pediatric Radius Fracture",
        /* Waiting (10) */
        "Multiple Trauma Concussion",  "Internal Bleeding Suspect", "Unstable Angina",
        "Anaphylactic Shock",          "Chronic Migraine",          "Viral Pyrexia",
        "Lumbar Disc Herniation",      "Gastric Ulcerative Bleed",  "Urinary Tract Infection",
        "Uncontrolled Hypertension",
        /* Discharged (5) */
        "Recovered Angina",            "Resolved Cholelithiasis",   "Treated Gastroenteritis",
        "Post-Appendectomy Recovery",  "Stabilized COPD Flare"
    };

    int ages[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        45, 58, 62, 29, 38, 67, 52, 48, 61, 70, 5, 8, 11, 3, 13,
        /* Waiting (10) */
        34, 27, 56, 42, 31, 40, 36, 50, 26, 47,
        /* Discharged (5) */
        54, 49, 22, 39, 65
    };

    char genders[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        'M', 'F', 'M', 'F', 'M', 'M', 'F', 'M', 'F', 'M', 'M', 'F', 'M', 'F', 'M',
        /* Waiting (10) */
        'M', 'M', 'M', 'F', 'F', 'M', 'F', 'M', 'F', 'O',
        /* Discharged (5) */
        'M', 'F', 'M', 'F', 'M'
    };

    int prios[DEMO_PATIENT_COUNT] = {
        /* Admitted (15): 8 Emergency (1), 7 Normal (2) */
        1, 2, 1, 2, 2, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2,
        /* Waiting (10): 4 Emergency (1), 6 Normal (2) */
        1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
        /* Discharged (5): 2 Emergency (1), 3 Normal (2) */
        1, 2, 2, 1, 2
    };

    int docIds[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        3, 2, 3, 2, 4, 3, 1, 1, 1, 3, 5, 5, 5, 5, 4,
        /* Waiting (10): 5 assigned, 5 unassigned (-1) */
        1, -1, -1, -1, 2, 2, 4, 2, -1, -1,
        /* Discharged (5) */
        3, 2, 2, 1, 2
    };

    int wards[DEMO_PATIENT_COUNT] = {
        /* Admitted (15): Ward 0 (General: 0..4), Ward 1 (ICU: 0..4), Ward 2 (Pediatric: 0..4) */
        0, 0, 0, 0, 0,
        1, 1, 1, 1, 1,
        2, 2, 2, 2, 2,
        /* Waiting (10) */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* Discharged (5) */
        -1, -1, -1, -1, -1
    };

    int bedNos[DEMO_PATIENT_COUNT] = {
        /* Admitted (15): 5 beds per ward (indices 0..4) */
        0, 1, 2, 3, 4,
        0, 1, 2, 3, 4,
        0, 1, 2, 3, 4,
        /* Waiting (10) */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* Discharged (5) */
        -1, -1, -1, -1, -1
    };

    int days[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        4, 5, 3, 2, 6, 7, 5, 4, 6, 8, 3, 4, 2, 3, 2,
        /* Waiting (10) */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* Discharged (5) */
        3, 4, 2, 5, 3
    };

    int statuses[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* Waiting (10) */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* Discharged (5) */
        2, 2, 2, 2, 2
    };

    float consults[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        500.0f, 500.0f, 600.0f, 400.0f, 500.0f,
        800.0f, 700.0f, 750.0f, 800.0f, 800.0f,
        500.0f, 500.0f, 450.0f, 450.0f, 500.0f,
        /* Waiting (10) */
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        /* Discharged (5) */
        500.0f, 600.0f, 400.0f, 500.0f, 500.0f
    };

    float meds[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        1400.0f, 900.0f, 1100.0f, 600.0f, 1800.0f,
        3200.0f, 2800.0f, 2600.0f, 3500.0f, 4000.0f,
        950.0f, 1100.0f, 650.0f, 700.0f, 800.0f,
        /* Waiting (10) */
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        /* Discharged (5) */
        800.0f, 1200.0f, 500.0f, 1500.0f, 900.0f
    };

    float roomRates[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        800.0f, 800.0f, 800.0f, 800.0f, 800.0f,
        2500.0f, 2500.0f, 2500.0f, 2500.0f, 2500.0f,
        1200.0f, 1200.0f, 1200.0f, 1200.0f, 1200.0f,
        /* Waiting (10) */
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        /* Discharged (5) */
        1000.0f, 1200.0f, 800.0f, 1500.0f, 900.0f
    };

    float testCosts[DEMO_PATIENT_COUNT] = {
        /* Admitted (15) */
        1200.0f, 600.0f, 900.0f, 400.0f, 1500.0f,
        2400.0f, 1800.0f, 2100.0f, 3000.0f, 2200.0f,
        700.0f, 900.0f, 500.0f, 400.0f, 850.0f,
        /* Waiting (10) */
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        /* Discharged (5) */
        600.0f, 1500.0f, 300.0f, 2000.0f, 800.0f
    };

    FILE *f = fopen(PATIENTS_FILE, "wb");
    if (f == NULL) { printError("Cannot create demo data file."); return; }

    int i;
    for (i = 0; i < DEMO_PATIENT_COUNT; i++) {
        struct Patient p;
        p.id = i + 1;
        strcpy(p.name,    names[i]);
        strcpy(p.disease, diseases[i]);
        p.age                  = ages[i];
        p.gender               = genders[i];
        p.priority             = prios[i];
        p.doctorId             = docIds[i];
        p.wardNo               = wards[i];
        p.bedNo                = bedNos[i];
        p.daysAdmitted         = days[i];
        p.status               = statuses[i];
        p.charges.consultation = consults[i];
        p.charges.medicine     = meds[i];
        p.charges.roomPerDay   = roomRates[i];
        p.charges.tests        = testCosts[i];

        if (p.status == 1 && p.wardNo >= 0 && p.bedNo >= 0) {
            beds[p.wardNo][p.bedNo] = 1;
        }

        if (p.status != 2 && p.doctorId >= 1 && p.doctorId <= MAX_DOCTORS) {
            doctors[p.doctorId - 1].patientCount++;
        }

        fwrite(&p, sizeof(struct Patient), 1, f);
    }
    fclose(f);
    saveBeds();
    printSuccess("30 demo patients loaded successfully.");
}

/* =============================================================
 *  main – Entry point; do-while loop with switch menu
 * ============================================================= */
int main(void)
{
    initDoctors();
    loadBeds();
    recalculateDoctorLoads();

    int choice;
    struct Patient tempP;  /* reusable temp patient for pointer passing */

    do {
        showMenu();
        scanf("%d", &choice);
        clearInputBuffer();

        switch (choice) {
            case 1:
                CLEAR();
                printSectionHeader("REGISTER PATIENT");
                registerPatient(&tempP);   /* POINTER USED HERE */
                pauseEnter();
                break;
            case 2:
                CLEAR();
                printSectionHeader("TRIAGE LIST  -  EMERGENCY FIRST");
                triageList();
                pauseEnter();
                break;
            case 3:
                CLEAR();
                printSectionHeader("ASSIGN DOCTOR TO PATIENT");
                assignDoctor();
                pauseEnter();
                break;
            case 4:
                CLEAR();
                printSectionHeader("BED GRID");
                showBeds(beds);            /* POINTER USED HERE */
                pauseEnter();
                break;
            case 5:
                CLEAR();
                printSectionHeader("ALLOCATE BED TO PATIENT");
                printf("  " CYAN "Patient ID: " RESET);
                scanf("%d", &tempP.id); clearInputBuffer();
                allocateBed(&tempP);       /* POINTER USED HERE */
                pauseEnter();
                break;
            case 6:
                CLEAR();
                printSectionHeader("ADD TREATMENT CHARGES");
                addCharges();
                pauseEnter();
                break;
            case 7:
                CLEAR();
                printSectionHeader("DISCHARGE PATIENT");
                dischargePatient(&tempP);  /* POINTER USED HERE */
                pauseEnter();
                break;
            case 8:
                CLEAR();
                printSectionHeader("SEARCH PATIENT");
                searchPatient();
                pauseEnter();
                break;
            case 9:
                CLEAR();
                printSectionHeader("UPDATE PATIENT DETAILS");
                updatePatient(&tempP);     /* POINTER USED HERE */
                pauseEnter();
                break;
            case 10:
                CLEAR();
                printSectionHeader("ALL PATIENTS");
                displayAll();
                pauseEnter();
                break;
            case 11:
                CLEAR();
                printSectionHeader("HOSPITAL SUMMARY DASHBOARD");
                hospitalSummary();
                pauseEnter();
                break;
            case 0:
                CLEAR();
                printf("\n");
                boxTop();
                boxLineCenter(GREEN BOLD, "Goodbye!  Stay Healthy.");
                boxBottom();
                printf("\n");
                break;
            case 99:
                /* Hidden demo loader – not listed in menu */
                CLEAR();
                printSectionHeader("DEMO DATA LOADER");
                loadDemoPatients();
                pauseEnter();
                break;
            default:
                printError("Invalid choice. Enter 0-11.");
                pauseEnter();
        }
    } while (choice != 0);

    return 0;
}
