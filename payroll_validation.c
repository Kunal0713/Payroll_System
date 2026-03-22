/*
 * payroll_validation.c
 * ─────────────────────────────────────────────────────────────────────────────
 * PayFlow — Input Validation & Data Integrity Module
 * All validation logic for employee data entry and record integrity.
 *
 * Compile: gcc -c payroll_validation.c -o payroll_validation.o
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_EMP_ID       99999
#define MIN_EMP_ID           1
#define MAX_OT_HOURS       744    /* 31 days × 24 hrs */
#define MAX_BASIC_PAY  9999999.0
#define MAX_NAME_LEN        50

/* ── Validation result ────────────────────────────────────────────────────── */
typedef struct {
    int  valid;
    char message[128];
} ValidationResult;

/* ── Helper: make a passing result ──────────────────────────────────────── */
static ValidationResult ok(void) {
    ValidationResult r;
    r.valid = 1;
    r.message[0] = '\0';
    return r;
}

/* ── Helper: make a failing result ──────────────────────────────────────── */
static ValidationResult fail(const char *msg) {
    ValidationResult r;
    r.valid = 0;
    strncpy(r.message, msg, sizeof(r.message) - 1);
    r.message[sizeof(r.message) - 1] = '\0';
    return r;
}

/* ── Validate Employee ID ─────────────────────────────────────────────────── */
ValidationResult validate_emp_id(int id) {
    if (id < MIN_EMP_ID)
        return fail("Employee ID must be at least 1.");
    if (id > MAX_EMP_ID)
        return fail("Employee ID must not exceed 99999.");
    return ok();
}

/* ── Validate Employee Name ───────────────────────────────────────────────── */
ValidationResult validate_name(const char *name) {
    if (!name || strlen(name) == 0)
        return fail("Name cannot be empty.");
    if (strlen(name) > MAX_NAME_LEN - 1)
        return fail("Name exceeds maximum length of 49 characters.");

    /* Must contain at least one alphabetic character */
    int has_alpha = 0;
    for (int i = 0; name[i]; i++)
        if (isalpha((unsigned char)name[i])) { has_alpha = 1; break; }
    if (!has_alpha)
        return fail("Name must contain at least one letter.");

    /* Reject names with only spaces */
    int all_space = 1;
    for (int i = 0; name[i]; i++)
        if (!isspace((unsigned char)name[i])) { all_space = 0; break; }
    if (all_space)
        return fail("Name cannot be all whitespace.");

    return ok();
}

/* ── Validate Basic Pay ───────────────────────────────────────────────────── */
ValidationResult validate_basic_pay(double pay) {
    if (pay < 0.0)
        return fail("Basic pay cannot be negative.");
    if (pay > MAX_BASIC_PAY)
        return fail("Basic pay exceeds maximum allowed value.");
    return ok();
}

/* ── Validate OT Hours ────────────────────────────────────────────────────── */
ValidationResult validate_ot_hours(double hours) {
    if (hours < 0.0)
        return fail("Overtime hours cannot be negative.");
    if (hours > MAX_OT_HOURS)
        return fail("Overtime hours exceed monthly maximum (744 hrs = 31 days x 24 hrs).");
    return ok();
}

/* ── Validate Department Code ─────────────────────────────────────────────── */
ValidationResult validate_department(int dept) {
    if (dept < 0 || dept > 4)
        return fail("Department must be 0=General, 1=Engineering, 2=Marketing, 3=Finance, 4=HR.");
    return ok();
}

/* ── Validate Join Date (YYYY-MM-DD) ─────────────────────────────────────── */
ValidationResult validate_join_date(const char *date) {
    if (!date || strlen(date) == 0)
        return ok(); /* optional field */

    if (strlen(date) != 10)
        return fail("Join date must be in YYYY-MM-DD format.");

    /* Check separators */
    if (date[4] != '-' || date[7] != '-')
        return fail("Join date must use '-' separators: YYYY-MM-DD.");

    /* Check all digits in correct positions */
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)date[i]))
            return fail("Join date contains invalid characters.");
    }

    int year  = (date[0]-'0')*1000 + (date[1]-'0')*100 +
                (date[2]-'0')*10   + (date[3]-'0');
    int month = (date[5]-'0')*10   + (date[6]-'0');
    int day   = (date[8]-'0')*10   + (date[9]-'0');

    if (year < 1900 || year > 2100) return fail("Year out of valid range (1900-2100).");
    if (month < 1   || month > 12)  return fail("Month must be between 01 and 12.");
    if (day   < 1   || day   > 31)  return fail("Day must be between 01 and 31.");

    return ok();
}

/* ── Validate full employee record ───────────────────────────────────────── */
ValidationResult validate_employee_record(int id, const char *name,
                                           double basic_pay, double ot_hours,
                                           int department, const char *join_date) {
    ValidationResult r;

    r = validate_emp_id(id);      if (!r.valid) return r;
    r = validate_name(name);      if (!r.valid) return r;
    r = validate_basic_pay(basic_pay);  if (!r.valid) return r;
    r = validate_ot_hours(ot_hours);    if (!r.valid) return r;
    r = validate_department(department);if (!r.valid) return r;
    if (join_date && strlen(join_date) > 0) {
        r = validate_join_date(join_date);
        if (!r.valid) return r;
    }

    return ok();
}

/* ── Check for duplicate ID in array ────────────────────────────────────── */
int is_duplicate_id(int *ids, int count, int new_id) {
    for (int i = 0; i < count; i++)
        if (ids[i] == new_id) return 1;
    return 0;
}

/* ── Sanitise a name string ──────────────────────────────────────────────── */
void sanitise_name(char *name) {
    if (!name) return;

    /* Trim leading whitespace */
    int start = 0;
    while (name[start] && isspace((unsigned char)name[start])) start++;
    if (start > 0) memmove(name, name + start, strlen(name) - start + 1);

    /* Trim trailing whitespace */
    int len = (int)strlen(name);
    while (len > 0 && isspace((unsigned char)name[len - 1]))
        name[--len] = '\0';

    /* Collapse multiple internal spaces */
    int write = 0;
    int in_space = 0;
    for (int read = 0; name[read]; read++) {
        if (isspace((unsigned char)name[read])) {
            if (!in_space && write > 0) name[write++] = ' ';
            in_space = 1;
        } else {
            name[write++] = name[read];
            in_space = 0;
        }
    }
    name[write] = '\0';
}

/* ── Print validation result ─────────────────────────────────────────────── */
void print_validation_result(ValidationResult r, const char *field) {
    if (r.valid)
        printf("  [PASS] %s\n", field);
    else
        printf("  [FAIL] %s: %s\n", field, r.message);
}

/* ── Run validation test suite ───────────────────────────────────────────── */
void run_validation_tests(void) {
    printf("\n[PayFlow] Running Validation Test Suite\n");
    printf("─────────────────────────────────────────\n");

    struct { const char *label; ValidationResult r; } tests[] = {
        { "Valid ID (1001)",          validate_emp_id(1001) },
        { "Invalid ID (0)",           validate_emp_id(0) },
        { "Invalid ID (-5)",          validate_emp_id(-5) },
        { "Valid Name",               validate_name("Kunal Deshmukh") },
        { "Empty Name",               validate_name("") },
        { "Spaces Only Name",         validate_name("   ") },
        { "Valid Basic Pay",          validate_basic_pay(45000.0) },
        { "Negative Pay",             validate_basic_pay(-1000.0) },
        { "Zero Pay (boundary)",      validate_basic_pay(0.0) },
        { "Valid OT (10h)",           validate_ot_hours(10.0) },
        { "Negative OT",              validate_ot_hours(-1.0) },
        { "OT at boundary (744h)",    validate_ot_hours(744.0) },
        { "OT over boundary (745h)",  validate_ot_hours(745.0) },
        { "Valid Date",               validate_join_date("2022-06-15") },
        { "Bad Date format",          validate_join_date("15-06-2022") },
        { "Invalid month (13)",       validate_join_date("2022-13-01") },
    };

    int pass = 0, fail_count = 0;
    int n = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < n; i++) {
        print_validation_result(tests[i].r, tests[i].label);
        if (tests[i].r.valid) pass++; else fail_count++;
    }

    printf("─────────────────────────────────────────\n");
    printf("  Results: %d passed, %d failed (expected failures are correct)\n\n",
           pass, fail_count);
}
