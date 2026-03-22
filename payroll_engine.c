/*
 * payroll_engine.c
 * ─────────────────────────────────────────────────────────────────────────────
 * PayFlow — Core Payroll Calculation Engine
 * The browser JS (index.html) is a faithful port of this C engine.
 *
 * Compile:  gcc -o payroll_engine payroll_engine.c -lm
 * Usage:    ./payroll_engine
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ── Constants ────────────────────────────────────────────────────────────── */
#define MAX_EMPLOYEES     100
#define MAX_NAME_LEN       50
#define OT_RATE          150.0
#define TAX_BRACKET_LOW    0.05
#define TAX_BRACKET_MID    0.10
#define TAX_BRACKET_HIGH   0.15
#define THRESHOLD_LOW   30000.0
#define THRESHOLD_HIGH  60000.0
#define VERSION         "1.0.0"

/* ── ANSI colour codes for terminal output ───────────────────────────────── */
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_GREEN   "\033[32m"
#define CLR_CYAN    "\033[36m"
#define CLR_YELLOW  "\033[33m"
#define CLR_RED     "\033[31m"
#define CLR_MAGENTA "\033[35m"

/* ── Data structures ──────────────────────────────────────────────────────── */
typedef enum {
    DEPT_GENERAL = 0,
    DEPT_ENGINEERING,
    DEPT_MARKETING,
    DEPT_FINANCE,
    DEPT_HR
} Department;

typedef enum {
    STATUS_ACTIVE = 0,
    STATUS_INACTIVE
} EmployeeStatus;

typedef struct {
    int            emp_id;
    char           name[MAX_NAME_LEN];
    Department     department;
    EmployeeStatus status;
    double         basic_pay;
    double         ot_hours;
    double         gross_pay;
    double         tax;
    double         net_pay;
    double         tax_rate;
    char           join_date[11];   /* YYYY-MM-DD */
} Employee;

typedef struct {
    Employee employees[MAX_EMPLOYEES];
    int      count;
    char     org_name[100];
    char     period[20];
} PayrollDB;

typedef struct {
    double total_gross;
    double total_tax;
    double total_net;
    double average_net;
    double highest_net;
    double lowest_net;
    int    active_count;
    int    inactive_count;
} PayrollSummary;

/* ── Error codes ──────────────────────────────────────────────────────────── */
typedef enum {
    ERR_OK          =  0,
    ERR_DB_FULL     = -1,
    ERR_DUPLICATE   = -2,
    ERR_NOT_FOUND   = -3,
    ERR_INVALID_ARG = -4
} ErrorCode;

/* ── Department name lookup ───────────────────────────────────────────────── */
const char *dept_name(Department d) {
    switch (d) {
        case DEPT_ENGINEERING: return "Engineering";
        case DEPT_MARKETING:   return "Marketing";
        case DEPT_FINANCE:     return "Finance";
        case DEPT_HR:          return "HR";
        default:               return "General";
    }
}

/* ── Tax bracket label ────────────────────────────────────────────────────── */
const char *tax_bracket_label(double gross) {
    if (gross < THRESHOLD_LOW)  return "5%";
    if (gross < THRESHOLD_HIGH) return "10%";
    return "15%";
}

/* ── Core payroll calculation ─────────────────────────────────────────────── */
void calc_employee(Employee *e) {
    if (!e) return;

    e->gross_pay = e->basic_pay + (e->ot_hours * OT_RATE);

    if (e->gross_pay < THRESHOLD_LOW) {
        e->tax_rate = TAX_BRACKET_LOW;
    } else if (e->gross_pay < THRESHOLD_HIGH) {
        e->tax_rate = TAX_BRACKET_MID;
    } else {
        e->tax_rate = TAX_BRACKET_HIGH;
    }

    e->tax     = e->gross_pay * e->tax_rate;
    e->net_pay = e->gross_pay - e->tax;
}

/* ── Add employee ─────────────────────────────────────────────────────────── */
ErrorCode add_employee(PayrollDB *db, int id, const char *name,
                       Department dept, double basic, double ot,
                       const char *join_date) {
    if (!db || !name)              return ERR_INVALID_ARG;
    if (db->count >= MAX_EMPLOYEES) return ERR_DB_FULL;

    for (int i = 0; i < db->count; i++)
        if (db->employees[i].emp_id == id) return ERR_DUPLICATE;

    Employee *e = &db->employees[db->count++];
    e->emp_id    = id;
    e->department = dept;
    e->status    = STATUS_ACTIVE;
    e->basic_pay = basic;
    e->ot_hours  = ot;
    strncpy(e->name,      name,      MAX_NAME_LEN - 1);
    strncpy(e->join_date, join_date, sizeof(e->join_date) - 1);
    calc_employee(e);
    return ERR_OK;
}

/* ── Remove employee ──────────────────────────────────────────────────────── */
ErrorCode remove_employee(PayrollDB *db, int id) {
    if (!db) return ERR_INVALID_ARG;
    for (int i = 0; i < db->count; i++) {
        if (db->employees[i].emp_id == id) {
            /* Shift remaining employees left */
            memmove(&db->employees[i], &db->employees[i + 1],
                    (db->count - i - 1) * sizeof(Employee));
            db->count--;
            return ERR_OK;
        }
    }
    return ERR_NOT_FOUND;
}

/* ── Find employee by ID ──────────────────────────────────────────────────── */
Employee *find_employee(PayrollDB *db, int id) {
    if (!db) return NULL;
    for (int i = 0; i < db->count; i++)
        if (db->employees[i].emp_id == id)
            return &db->employees[i];
    return NULL;
}

/* ── Update employee basic pay ────────────────────────────────────────────── */
ErrorCode update_basic_pay(PayrollDB *db, int id, double new_basic) {
    Employee *e = find_employee(db, id);
    if (!e) return ERR_NOT_FOUND;
    e->basic_pay = new_basic;
    calc_employee(e);
    return ERR_OK;
}

/* ── Aggregate calculations ───────────────────────────────────────────────── */
double total_net_payroll(const PayrollDB *db) {
    double total = 0.0;
    for (int i = 0; i < db->count; i++)
        if (db->employees[i].status == STATUS_ACTIVE)
            total += db->employees[i].net_pay;
    return total;
}

double total_gross_payroll(const PayrollDB *db) {
    double total = 0.0;
    for (int i = 0; i < db->count; i++)
        total += db->employees[i].gross_pay;
    return total;
}

double total_tax_collected(const PayrollDB *db) {
    double total = 0.0;
    for (int i = 0; i < db->count; i++)
        total += db->employees[i].tax;
    return total;
}

double average_salary(const PayrollDB *db) {
    int active = 0;
    double total = 0.0;
    for (int i = 0; i < db->count; i++) {
        if (db->employees[i].status == STATUS_ACTIVE) {
            total += db->employees[i].net_pay;
            active++;
        }
    }
    return active ? total / active : 0.0;
}

Employee *highest_earner(PayrollDB *db) {
    if (!db || db->count == 0) return NULL;
    Employee *hi = &db->employees[0];
    for (int i = 1; i < db->count; i++)
        if (db->employees[i].net_pay > hi->net_pay)
            hi = &db->employees[i];
    return hi;
}

Employee *lowest_earner(PayrollDB *db) {
    if (!db || db->count == 0) return NULL;
    Employee *lo = &db->employees[0];
    for (int i = 1; i < db->count; i++)
        if (db->employees[i].net_pay < lo->net_pay)
            lo = &db->employees[i];
    return lo;
}

PayrollSummary compute_summary(PayrollDB *db) {
    PayrollSummary s = {0};
    if (!db || db->count == 0) return s;

    s.highest_net = -1e18;
    s.lowest_net  =  1e18;

    for (int i = 0; i < db->count; i++) {
        Employee *e = &db->employees[i];
        s.total_gross += e->gross_pay;
        s.total_tax   += e->tax;
        s.total_net   += e->net_pay;
        if (e->net_pay > s.highest_net) s.highest_net = e->net_pay;
        if (e->net_pay < s.lowest_net)  s.lowest_net  = e->net_pay;
        if (e->status == STATUS_ACTIVE) s.active_count++;
        else                            s.inactive_count++;
    }
    s.average_net = db->count ? s.total_net / db->count : 0.0;
    return s;
}

/* ── Sorting helpers ──────────────────────────────────────────────────────── */
int cmp_net_pay_desc(const void *a, const void *b) {
    const Employee *ea = (const Employee *)a;
    const Employee *eb = (const Employee *)b;
    if (eb->net_pay > ea->net_pay) return  1;
    if (eb->net_pay < ea->net_pay) return -1;
    return 0;
}

int cmp_emp_id_asc(const void *a, const void *b) {
    return ((const Employee *)a)->emp_id - ((const Employee *)b)->emp_id;
}

void sort_by_net_pay(PayrollDB *db) {
    qsort(db->employees, db->count, sizeof(Employee), cmp_net_pay_desc);
}

void sort_by_id(PayrollDB *db) {
    qsort(db->employees, db->count, sizeof(Employee), cmp_emp_id_asc);
}

/* ── Print helpers ────────────────────────────────────────────────────────── */
void print_divider(int width) {
    for (int i = 0; i < width; i++) putchar('─');
    putchar('\n');
}

void print_payslip(const Employee *e) {
    printf("\n");
    printf(CLR_CYAN "╔══════════════════════════════════════════╗\n" CLR_RESET);
    printf(CLR_CYAN "║" CLR_BOLD "        PayFlow — PAY SLIP  v%-13s" CLR_RESET CLR_CYAN "║\n" CLR_RESET, VERSION);
    printf(CLR_CYAN "╠══════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN "║" CLR_RESET "  Employee  : %-28s" CLR_CYAN "║\n" CLR_RESET, e->name);
    printf(CLR_CYAN "║" CLR_RESET "  ID        : #%-27d" CLR_CYAN "║\n" CLR_RESET, e->emp_id);
    printf(CLR_CYAN "║" CLR_RESET "  Department: %-28s" CLR_CYAN "║\n" CLR_RESET, dept_name(e->department));
    printf(CLR_CYAN "║" CLR_RESET "  Joined    : %-28s" CLR_CYAN "║\n" CLR_RESET, e->join_date);
    printf(CLR_CYAN "╠══════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN "║" CLR_RESET "  Basic Pay      : " CLR_GREEN "Rs. %18.2f" CLR_RESET CLR_CYAN " ║\n" CLR_RESET, e->basic_pay);
    printf(CLR_CYAN "║" CLR_RESET "  OT (%.0fh×Rs.%g)  : " CLR_GREEN "Rs. %18.2f" CLR_RESET CLR_CYAN " ║\n" CLR_RESET,
           e->ot_hours, OT_RATE, e->ot_hours * OT_RATE);
    printf(CLR_CYAN "║" CLR_RESET "  Gross Pay      : " CLR_YELLOW "Rs. %18.2f" CLR_RESET CLR_CYAN " ║\n" CLR_RESET, e->gross_pay);
    printf(CLR_CYAN "║" CLR_RESET "  Tax (%s)       : " CLR_RED "Rs. %18.2f" CLR_RESET CLR_CYAN " ║\n" CLR_RESET,
           tax_bracket_label(e->gross_pay), e->tax);
    printf(CLR_CYAN "╠══════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN "║" CLR_RESET "  " CLR_BOLD "NET PAY          : " CLR_GREEN "Rs. %18.2f" CLR_RESET CLR_CYAN " ║\n" CLR_RESET, e->net_pay);
    printf(CLR_CYAN "╚══════════════════════════════════════════╝\n" CLR_RESET);
}

void print_report(PayrollDB *db) {
    PayrollSummary s = compute_summary(db);

    printf("\n" CLR_BOLD CLR_MAGENTA
           "══════════════════  PAYFLOW PAYROLL REPORT  ══════════════════\n"
           CLR_RESET);
    printf("  Organisation : %s\n", db->org_name);
    printf("  Period       : %s\n", db->period);
    printf("  Employees    : %d active\n\n", s.active_count);

    printf(CLR_BOLD "  %-5s %-22s %-14s %12s %10s %12s\n" CLR_RESET,
           "ID", "Name", "Department", "Gross", "Tax", "Net Pay");
    print_divider(80);

    for (int i = 0; i < db->count; i++) {
        Employee *e = &db->employees[i];
        printf("  %-5d %-22s %-14s %12.2f %10.2f " CLR_GREEN "%12.2f" CLR_RESET "\n",
               e->emp_id, e->name, dept_name(e->department),
               e->gross_pay, e->tax, e->net_pay);
    }

    print_divider(80);
    printf("  %-5s %-22s %-14s %12.2f %10.2f " CLR_GREEN "%12.2f\n" CLR_RESET,
           "", "TOTAL", "", s.total_gross, s.total_tax, s.total_net);

    printf("\n  " CLR_BOLD "Summary\n" CLR_RESET);
    printf("  Average Net Pay  : Rs. %.2f\n", s.average_net);

    Employee *hi = highest_earner(db);
    Employee *lo = lowest_earner(db);
    if (hi) printf("  Highest Earner   : %s (Rs. %.2f)\n", hi->name, hi->net_pay);
    if (lo) printf("  Lowest Earner    : %s (Rs. %.2f)\n", lo->name, lo->net_pay);

    printf(CLR_MAGENTA
           "═══════════════════════════════════════════════════════════════\n\n"
           CLR_RESET);
}

/* ── Demo / main ──────────────────────────────────────────────────────────── */
int main(void) {
    PayrollDB db = { .count = 0 };
    strncpy(db.org_name, "PayFlow Technologies Pvt. Ltd.", sizeof(db.org_name)-1);
    strncpy(db.period,   "March 2026",                     sizeof(db.period)-1);

    /* Seed with same employees as the web dashboard */
    add_employee(&db, 1001, "Aarav Sharma",  DEPT_ENGINEERING, 45000.0, 12.0, "2022-06-01");
    add_employee(&db, 1002, "Priya Patel",   DEPT_HR,          32000.0,  8.0, "2023-01-15");
    add_employee(&db, 1003, "Rohit Verma",   DEPT_FINANCE,     68000.0, 20.0, "2021-09-10");
    add_employee(&db, 1004, "Sneha Iyer",    DEPT_MARKETING,   27500.0,  5.0, "2024-03-01");
    add_employee(&db, 1005, "Karthik Nair",  DEPT_ENGINEERING, 55000.0, 15.0, "2022-11-20");
    add_employee(&db, 1006, "Divya Menon",   DEPT_GENERAL,     48000.0, 10.0, "2023-07-05");

    /* Print full report */
    print_report(&db);

    /* Print individual payslip */
    Employee *e = find_employee(&db, 1003);
    if (e) print_payslip(e);

    /* Demo: sort by net pay and reprint summary */
    sort_by_net_pay(&db);
    printf(CLR_BOLD "  Ranked by Net Pay:\n" CLR_RESET);
    for (int i = 0; i < db.count; i++)
        printf("  %d. %-22s Rs. %.2f\n",
               i + 1, db.employees[i].name, db.employees[i].net_pay);

    /* Demo: update a salary */
    printf("\n  Updating Aarav Sharma's basic pay to Rs. 52,000...\n");
    update_basic_pay(&db, 1001, 52000.0);
    e = find_employee(&db, 1001);
    if (e) printf("  New net pay: Rs. %.2f\n\n", e->net_pay);

    return 0;
}
