/*
 * payroll_reports.c
 * ─────────────────────────────────────────────────────────────────────────────
 * PayFlow — Payroll Report Generator Module
 * Handles all report generation, formatting, and statistical analysis.
 *
 * Compile: gcc -c payroll_reports.c -o payroll_reports.o
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_EMPLOYEES   100
#define OT_RATE         150.0
#define THRESHOLD_LOW   30000.0
#define THRESHOLD_HIGH  60000.0
#define REPORT_WIDTH    78

/* ── ANSI colours ─────────────────────────────────────────────────────────── */
#define CLR_RESET    "\033[0m"
#define CLR_BOLD     "\033[1m"
#define CLR_GREEN    "\033[32m"
#define CLR_CYAN     "\033[36m"
#define CLR_YELLOW   "\033[33m"
#define CLR_RED      "\033[31m"
#define CLR_MAGENTA  "\033[35m"
#define CLR_BLUE     "\033[34m"
#define CLR_WHITE    "\033[37m"

/* ── Report types ─────────────────────────────────────────────────────────── */
typedef enum {
    REPORT_SUMMARY    = 1,
    REPORT_DETAILED   = 2,
    REPORT_TAX        = 3,
    REPORT_DEPARTMENT = 4,
    REPORT_PAYSLIP    = 5
} ReportType;

/* ── Employee record (mirrors payroll_engine.c) ───────────────────────────── */
typedef struct {
    int    emp_id;
    char   name[50];
    int    department;   /* 0=General,1=Eng,2=Mkt,3=Finance,4=HR */
    int    status;       /* 0=active, 1=inactive */
    double basic_pay;
    double ot_hours;
    double gross_pay;
    double tax;
    double net_pay;
    double tax_rate;
    char   join_date[11];
} ReportEmployee;

/* ── Report statistics ────────────────────────────────────────────────────── */
typedef struct {
    int    total_employees;
    int    active_count;
    double total_gross;
    double total_tax;
    double total_net;
    double average_gross;
    double average_net;
    double highest_net;
    double lowest_net;
    int    highest_earner_id;
    int    lowest_earner_id;
    int    tax_bracket_5;   /* employees in 5% bracket  */
    int    tax_bracket_10;  /* employees in 10% bracket */
    int    tax_bracket_15;  /* employees in 15% bracket */
    double dept_totals[5];  /* net pay per department   */
} ReportStats;

/* ── Department names ─────────────────────────────────────────────────────── */
static const char *dept_labels[] = {
    "General", "Engineering", "Marketing", "Finance", "HR"
};

/* ── Helper: print a horizontal rule ─────────────────────────────────────── */
static void hr(char ch, int width) {
    for (int i = 0; i < width; i++) putchar(ch);
    putchar('\n');
}

/* ── Helper: centre a string in a given width ────────────────────────────── */
static void print_centred(const char *text, int width) {
    int len = (int)strlen(text);
    int pad = (width - len) / 2;
    for (int i = 0; i < pad; i++) putchar(' ');
    printf("%s\n", text);
}

/* ── Compute statistics from employee array ──────────────────────────────── */
ReportStats compute_stats(ReportEmployee *emps, int count) {
    ReportStats s = {0};
    if (!emps || count == 0) return s;

    s.total_employees = count;
    s.highest_net = -1e18;
    s.lowest_net  =  1e18;

    for (int i = 0; i < count; i++) {
        ReportEmployee *e = &emps[i];
        if (e->status != 0) continue;
        s.active_count++;
        s.total_gross += e->gross_pay;
        s.total_tax   += e->tax;
        s.total_net   += e->net_pay;

        if (e->net_pay > s.highest_net) {
            s.highest_net = e->net_pay;
            s.highest_earner_id = e->emp_id;
        }
        if (e->net_pay < s.lowest_net) {
            s.lowest_net = e->net_pay;
            s.lowest_earner_id = e->emp_id;
        }
        if (e->gross_pay < THRESHOLD_LOW)       s.tax_bracket_5++;
        else if (e->gross_pay < THRESHOLD_HIGH) s.tax_bracket_10++;
        else                                    s.tax_bracket_15++;

        if (e->department >= 0 && e->department < 5)
            s.dept_totals[e->department] += e->net_pay;
    }

    if (s.active_count > 0) {
        s.average_gross = s.total_gross / s.active_count;
        s.average_net   = s.total_net   / s.active_count;
    }
    return s;
}

/* ── Format currency ──────────────────────────────────────────────────────── */
static void fmt_currency(char *buf, size_t sz, double amount) {
    snprintf(buf, sz, "Rs.%12.2f", amount);
}

/* ── Report 1: Executive Summary ─────────────────────────────────────────── */
void print_summary_report(ReportEmployee *emps, int count,
                           const char *org, const char *period) {
    ReportStats s = compute_stats(emps, count);
    char buf[32];

    printf("\n");
    hr('=', REPORT_WIDTH);
    printf(CLR_BOLD CLR_CYAN);
    print_centred("P A Y F L O W   —   E X E C U T I V E   S U M M A R Y", REPORT_WIDTH);
    printf(CLR_RESET);
    hr('=', REPORT_WIDTH);
    printf("  Organisation : %s\n", org);
    printf("  Period        : %s\n", period);
    printf("  Generated     : (see system date)\n");
    hr('-', REPORT_WIDTH);

    printf("\n  " CLR_BOLD "Workforce\n" CLR_RESET);
    printf("  Total Employees  : %d\n", s.total_employees);
    printf("  Active           : %d\n", s.active_count);
    printf("  Inactive         : %d\n", s.total_employees - s.active_count);

    printf("\n  " CLR_BOLD "Payroll Financials\n" CLR_RESET);
    fmt_currency(buf, sizeof(buf), s.total_gross);
    printf("  Total Gross Pay  : %s\n", buf);
    fmt_currency(buf, sizeof(buf), s.total_tax);
    printf("  Total Tax Deducted: %s\n", buf);
    fmt_currency(buf, sizeof(buf), s.total_net);
    printf("  " CLR_GREEN "Total Net Payroll : %s\n" CLR_RESET, buf);
    fmt_currency(buf, sizeof(buf), s.average_net);
    printf("  Average Net Pay  : %s\n", buf);

    printf("\n  " CLR_BOLD "Earnings Range\n" CLR_RESET);
    fmt_currency(buf, sizeof(buf), s.highest_net);
    printf("  Highest Net Pay  : %s  (ID #%d)\n", buf, s.highest_earner_id);
    fmt_currency(buf, sizeof(buf), s.lowest_net);
    printf("  Lowest Net Pay   : %s  (ID #%d)\n", buf, s.lowest_earner_id);

    printf("\n  " CLR_BOLD "Tax Bracket Distribution\n" CLR_RESET);
    int total = s.tax_bracket_5 + s.tax_bracket_10 + s.tax_bracket_15;
    if (total > 0) {
        printf("  5%%  bracket (< Rs.30,000)  : %2d employees  (%.0f%%)\n",
               s.tax_bracket_5,  100.0 * s.tax_bracket_5  / total);
        printf("  10%% bracket (Rs.30k-60k)   : %2d employees  (%.0f%%)\n",
               s.tax_bracket_10, 100.0 * s.tax_bracket_10 / total);
        printf("  15%% bracket (>= Rs.60,000) : %2d employees  (%.0f%%)\n",
               s.tax_bracket_15, 100.0 * s.tax_bracket_15 / total);
    }

    printf("\n  " CLR_BOLD "Department Net Pay\n" CLR_RESET);
    for (int d = 0; d < 5; d++) {
        if (s.dept_totals[d] > 0) {
            fmt_currency(buf, sizeof(buf), s.dept_totals[d]);
            printf("  %-14s : %s\n", dept_labels[d], buf);
        }
    }

    hr('=', REPORT_WIDTH);
    printf("\n");
}

/* ── Report 2: Full Detailed Employee Table ──────────────────────────────── */
void print_detailed_report(ReportEmployee *emps, int count,
                            const char *org, const char *period) {
    if (!emps || count == 0) { printf("No employee data.\n"); return; }

    printf("\n");
    hr('=', REPORT_WIDTH);
    printf(CLR_BOLD CLR_MAGENTA);
    print_centred("P A Y F L O W   —   D E T A I L E D   P A Y R O L L", REPORT_WIDTH);
    printf(CLR_RESET);
    hr('=', REPORT_WIDTH);
    printf("  %-5s  %-20s  %-12s  %10s  %8s  %10s\n",
           "ID", "Name", "Department", "Gross", "Tax", "Net Pay");
    hr('-', REPORT_WIDTH);

    for (int i = 0; i < count; i++) {
        ReportEmployee *e = &emps[i];
        const char *dept = (e->department >= 0 && e->department < 5)
                           ? dept_labels[e->department] : "Unknown";
        printf("  %-5d  %-20s  %-12s  %10.2f  %8.2f  "
               CLR_GREEN "%10.2f" CLR_RESET "\n",
               e->emp_id, e->name, dept,
               e->gross_pay, e->tax, e->net_pay);
    }

    hr('-', REPORT_WIDTH);
    ReportStats s = compute_stats(emps, count);
    printf("  %-5s  %-20s  %-12s  %10.2f  %8.2f  " CLR_BOLD "%10.2f\n" CLR_RESET,
           "—", "TOTAL", "", s.total_gross, s.total_tax, s.total_net);
    hr('=', REPORT_WIDTH);
    printf("\n");
}

/* ── Report 3: Tax Analysis Report ──────────────────────────────────────── */
void print_tax_report(ReportEmployee *emps, int count, const char *period) {
    if (!emps || count == 0) { printf("No data.\n"); return; }

    printf("\n");
    hr('=', REPORT_WIDTH);
    printf(CLR_BOLD CLR_YELLOW);
    print_centred("P A Y F L O W   —   T A X   A N A L Y S I S", REPORT_WIDTH);
    printf(CLR_RESET);
    printf(CLR_YELLOW "  Period: %s\n" CLR_RESET, period);
    hr('-', REPORT_WIDTH);
    printf("  %-5s  %-20s  %12s  %8s  %12s  %6s\n",
           "ID", "Name", "Gross Pay", "Rate", "Tax", "Net");
    hr('-', REPORT_WIDTH);

    double total_tax = 0.0;
    for (int i = 0; i < count; i++) {
        ReportEmployee *e = &emps[i];
        const char *rate = e->gross_pay < THRESHOLD_LOW  ? " 5%" :
                           e->gross_pay < THRESHOLD_HIGH ? "10%" : "15%";
        printf("  %-5d  %-20s  %12.2f  %8s  "
               CLR_RED "%12.2f" CLR_RESET "  %6.2f\n",
               e->emp_id, e->name,
               e->gross_pay, rate,
               e->tax, e->net_pay);
        total_tax += e->tax;
    }
    hr('-', REPORT_WIDTH);
    printf("  %-5s  %-20s  %12s  %8s  " CLR_BOLD CLR_RED "%12.2f\n" CLR_RESET,
           "—", "TOTAL TAX COLLECTED", "", "", total_tax);
    hr('=', REPORT_WIDTH);
    printf("\n");
}

/* ── Report 4: Department Breakdown ─────────────────────────────────────── */
void print_department_report(ReportEmployee *emps, int count) {
    if (!emps || count == 0) { printf("No data.\n"); return; }

    printf("\n");
    hr('=', REPORT_WIDTH);
    printf(CLR_BOLD CLR_BLUE);
    print_centred("P A Y F L O W   —   D E P A R T M E N T   B R E A K D O W N", REPORT_WIDTH);
    printf(CLR_RESET);
    hr('-', REPORT_WIDTH);

    for (int d = 0; d < 5; d++) {
        int dept_count = 0;
        double dept_net = 0.0, dept_gross = 0.0;

        for (int i = 0; i < count; i++) {
            if (emps[i].department == d) {
                dept_count++;
                dept_net   += emps[i].net_pay;
                dept_gross += emps[i].gross_pay;
            }
        }
        if (dept_count == 0) continue;

        printf("\n  " CLR_BOLD "%-14s" CLR_RESET "  (%d employees)\n",
               dept_labels[d], dept_count);
        printf("    Gross Payroll : Rs. %10.2f\n", dept_gross);
        printf("    Net Payroll   : Rs. %10.2f\n", dept_net);
        printf("    Avg Net Pay   : Rs. %10.2f\n",
               dept_count ? dept_net / dept_count : 0.0);

        /* ASCII bar chart */
        int bar_len = (int)(dept_net / 5000.0);
        if (bar_len > 40) bar_len = 40;
        printf("    [");
        for (int b = 0; b < bar_len;  b++) printf(CLR_GREEN "█" CLR_RESET);
        for (int b = bar_len; b < 40; b++) printf("░");
        printf("]\n");
    }
    hr('=', REPORT_WIDTH);
    printf("\n");
}

/* ── Report 5: Individual Payslip ────────────────────────────────────────── */
void print_payslip_report(const ReportEmployee *e, const char *period) {
    if (!e) return;
    const char *dept = (e->department >= 0 && e->department < 5)
                       ? dept_labels[e->department] : "General";
    const char *rate = e->gross_pay < THRESHOLD_LOW  ? "5%"  :
                       e->gross_pay < THRESHOLD_HIGH ? "10%" : "15%";

    printf("\n");
    printf(CLR_CYAN  "  ╔══════════════════════════════════════════════════╗\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_BOLD "              PayFlow — PAY SLIP                  " CLR_RESET CLR_CYAN "║\n" CLR_RESET);
    printf(CLR_CYAN  "  ╠══════════════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Period     : %-35s" CLR_CYAN "║\n" CLR_RESET, period);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Employee   : %-35s" CLR_CYAN "║\n" CLR_RESET, e->name);
    printf(CLR_CYAN  "  ║" CLR_RESET "  ID         : #%-34d" CLR_CYAN "║\n" CLR_RESET, e->emp_id);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Department : %-35s" CLR_CYAN "║\n" CLR_RESET, dept);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Joined     : %-35s" CLR_CYAN "║\n" CLR_RESET, e->join_date);
    printf(CLR_CYAN  "  ╠══════════════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  EARNINGS                                        " CLR_CYAN "║\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Basic Salary        : " CLR_GREEN "Rs. %20.2f" CLR_RESET CLR_CYAN "  ║\n" CLR_RESET, e->basic_pay);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Overtime (%.0fh@Rs.%.0f): " CLR_GREEN "Rs. %20.2f" CLR_RESET CLR_CYAN "  ║\n" CLR_RESET,
           e->ot_hours, OT_RATE, e->ot_hours * OT_RATE);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Gross Earnings      : " CLR_YELLOW "Rs. %20.2f" CLR_RESET CLR_CYAN "  ║\n" CLR_RESET, e->gross_pay);
    printf(CLR_CYAN  "  ╠══════════════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  DEDUCTIONS                                      " CLR_CYAN "║\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  Income Tax (%s)     : " CLR_RED "Rs. %20.2f" CLR_RESET CLR_CYAN "  ║\n" CLR_RESET, rate, e->tax);
    printf(CLR_CYAN  "  ╠══════════════════════════════════════════════════╣\n" CLR_RESET);
    printf(CLR_CYAN  "  ║" CLR_RESET "  " CLR_BOLD "NET TAKE-HOME PAY    : " CLR_GREEN "Rs. %20.2f" CLR_RESET CLR_CYAN "  ║\n" CLR_RESET, e->net_pay);
    printf(CLR_CYAN  "  ╚══════════════════════════════════════════════════╝\n" CLR_RESET);
    printf("\n");
}

/* ── Report dispatcher ────────────────────────────────────────────────────── */
void run_report(ReportType type, ReportEmployee *emps, int count,
                const char *org, const char *period, int target_id) {
    switch (type) {
        case REPORT_SUMMARY:
            print_summary_report(emps, count, org, period);
            break;
        case REPORT_DETAILED:
            print_detailed_report(emps, count, org, period);
            break;
        case REPORT_TAX:
            print_tax_report(emps, count, period);
            break;
        case REPORT_DEPARTMENT:
            print_department_report(emps, count);
            break;
        case REPORT_PAYSLIP:
            for (int i = 0; i < count; i++)
                if (emps[i].emp_id == target_id) {
                    print_payslip_report(&emps[i], period);
                    return;
                }
            printf("Employee #%d not found.\n", target_id);
            break;
        default:
            printf("Unknown report type.\n");
    }
}
