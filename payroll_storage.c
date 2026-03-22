/*
 * payroll_storage.c
 * ─────────────────────────────────────────────────────────────────────────────
 * PayFlow — File Persistence & Data Storage Module
 * Handles all CSV read/write operations for employee records.
 *
 * Compile: gcc -c payroll_storage.c -o payroll_storage.o
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EMPLOYEES   100
#define MAX_NAME_LEN     50
#define OT_RATE         150.0
#define THRESHOLD_LOW  30000.0
#define THRESHOLD_HIGH 60000.0
#define FILE_NAME      "employees.txt"
#define BACKUP_FILE    "employees_backup.txt"
#define EXPORT_FILE    "payroll_export.csv"

/* ── Storage result codes ─────────────────────────────────────────────────── */
typedef enum {
    STORE_OK           =  0,
    STORE_FILE_ERROR   = -1,
    STORE_READ_ERROR   = -2,
    STORE_WRITE_ERROR  = -3,
    STORE_CORRUPT      = -4,
    STORE_EMPTY        = -5
} StoreResult;

/* ── Employee record (mirrors payroll_engine.c) ───────────────────────────── */
typedef struct {
    int    emp_id;
    char   name[MAX_NAME_LEN];
    int    department;
    int    status;
    double basic_pay;
    double ot_hours;
    double gross_pay;
    double tax;
    double net_pay;
    double tax_rate;
    char   join_date[11];
} StorageEmployee;

/* ── Recompute derived fields ─────────────────────────────────────────────── */
static void recalc(StorageEmployee *e) {
    e->gross_pay = e->basic_pay + (e->ot_hours * OT_RATE);
    if      (e->gross_pay < THRESHOLD_LOW)  e->tax_rate = 0.05;
    else if (e->gross_pay < THRESHOLD_HIGH) e->tax_rate = 0.10;
    else                                    e->tax_rate = 0.15;
    e->tax     = e->gross_pay * e->tax_rate;
    e->net_pay = e->gross_pay - e->tax;
}

/* ── Save all employees to CSV file ──────────────────────────────────────── */
StoreResult save_employees(StorageEmployee *emps, int count, const char *filename) {
    if (!emps || !filename) return STORE_FILE_ERROR;
    if (count == 0)         return STORE_EMPTY;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[PayFlow] Error: Cannot open '%s' for writing.\n", filename);
        return STORE_WRITE_ERROR;
    }

    /* Write header row */
    fprintf(fp, "emp_id,name,department,status,basic_pay,ot_hours,join_date\n");

    int written = 0;
    for (int i = 0; i < count; i++) {
        StorageEmployee *e = &emps[i];
        int n = fprintf(fp, "%d,%s,%d,%d,%.2f,%.2f,%s\n",
                        e->emp_id, e->name, e->department, e->status,
                        e->basic_pay, e->ot_hours, e->join_date);
        if (n < 0) {
            fclose(fp);
            return STORE_WRITE_ERROR;
        }
        written++;
    }

    fclose(fp);
    printf("[PayFlow] Saved %d records to '%s'.\n", written, filename);
    return STORE_OK;
}

/* ── Load employees from CSV file ────────────────────────────────────────── */
StoreResult load_employees(StorageEmployee *emps, int *count, const char *filename) {
    if (!emps || !count || !filename) return STORE_FILE_ERROR;
    *count = 0;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("[PayFlow] No data file found. Starting fresh.\n");
        return STORE_FILE_ERROR;
    }

    char line[256];
    int  line_num = 0;

    /* Skip header row */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return STORE_EMPTY;
    }

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (*count >= MAX_EMPLOYEES) {
            fprintf(stderr, "[PayFlow] Warning: Max capacity (%d) reached.\n",
                    MAX_EMPLOYEES);
            break;
        }

        StorageEmployee e = {0};
        int parsed = sscanf(line, "%d,%49[^,],%d,%d,%lf,%lf,%10s",
                            &e.emp_id, e.name, &e.department, &e.status,
                            &e.basic_pay, &e.ot_hours, e.join_date);
        if (parsed < 6) {
            fprintf(stderr, "[PayFlow] Warning: Corrupt record at line %d — skipped.\n",
                    line_num + 1);
            continue;
        }
        if (e.join_date[0] == '\0') strncpy(e.join_date, "N/A", 4);

        recalc(&e);
        emps[(*count)++] = e;
    }

    fclose(fp);
    printf("[PayFlow] Loaded %d records from '%s'.\n", *count, filename);
    return *count > 0 ? STORE_OK : STORE_EMPTY;
}

/* ── Create a backup of the data file ───────────────────────────────────── */
StoreResult backup_file(void) {
    FILE *src = fopen(FILE_NAME,   "r");
    FILE *dst = fopen(BACKUP_FILE, "w");
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return STORE_FILE_ERROR;
    }

    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
        fwrite(buf, 1, n, dst);

    fclose(src);
    fclose(dst);
    printf("[PayFlow] Backup saved to '%s'.\n", BACKUP_FILE);
    return STORE_OK;
}

/* ── Export to CSV with full computed fields ─────────────────────────────── */
StoreResult export_csv(StorageEmployee *emps, int count,
                       const char *org, const char *period) {
    if (!emps || count == 0) return STORE_EMPTY;

    static const char *dept_names[] = {
        "General", "Engineering", "Marketing", "Finance", "HR"
    };
    static const char *tax_bracket(double g) {
        return g < 30000 ? "5%" : g < 60000 ? "10%" : "15%";
    }

    FILE *fp = fopen(EXPORT_FILE, "w");
    if (!fp) return STORE_WRITE_ERROR;

    /* CSV Header */
    fprintf(fp, "PayFlow Payroll Export\n");
    fprintf(fp, "Organisation,%s\n", org);
    fprintf(fp, "Period,%s\n\n", period);
    fprintf(fp, "ID,Name,Department,Join Date,Basic Pay,"
                "OT Hours,OT Pay,Gross Pay,Tax Rate,Tax,Net Pay\n");

    double total_net = 0.0, total_gross = 0.0, total_tax = 0.0;

    for (int i = 0; i < count; i++) {
        StorageEmployee *e = &emps[i];
        const char *dept = (e->department >= 0 && e->department < 5)
                           ? dept_names[e->department] : "General";
        double ot_pay = e->ot_hours * OT_RATE;

        fprintf(fp, "%d,\"%s\",%s,%s,%.2f,%.1f,%.2f,%.2f,%s,%.2f,%.2f\n",
                e->emp_id, e->name, dept, e->join_date,
                e->basic_pay, e->ot_hours, ot_pay,
                e->gross_pay, tax_bracket(e->gross_pay),
                e->tax, e->net_pay);

        total_gross += e->gross_pay;
        total_tax   += e->tax;
        total_net   += e->net_pay;
    }

    fprintf(fp, "\nSUMMARY\n");
    fprintf(fp, "Total Employees,%d\n", count);
    fprintf(fp, "Total Gross,%.2f\n",   total_gross);
    fprintf(fp, "Total Tax,%.2f\n",     total_tax);
    fprintf(fp, "Total Net,%.2f\n",     total_net);
    fprintf(fp, "Average Net,%.2f\n",
            count > 0 ? total_net / count : 0.0);

    fclose(fp);
    printf("[PayFlow] Exported %d records to '%s'.\n", count, EXPORT_FILE);
    return STORE_OK;
}

/* ── Check if a file exists ──────────────────────────────────────────────── */
int file_exists(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp) { fclose(fp); return 1; }
    return 0;
}

/* ── Get file size in bytes ──────────────────────────────────────────────── */
long file_size(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fclose(fp);
    return sz;
}

/* ── Count records in file without loading ───────────────────────────────── */
int count_records(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;

    int count = 0;
    char line[256];
    fgets(line, sizeof(line), fp); /* skip header */
    while (fgets(line, sizeof(line), fp))
        if (line[0] != '\n' && line[0] != '\r') count++;

    fclose(fp);
    return count;
}

/* ── Print storage diagnostics ───────────────────────────────────────────── */
void print_storage_info(void) {
    printf("\n[PayFlow] Storage Diagnostics\n");
    printf("  Data file    : %s  (%s, %ld bytes, %d records)\n",
           FILE_NAME,
           file_exists(FILE_NAME) ? "EXISTS" : "NOT FOUND",
           file_size(FILE_NAME),
           count_records(FILE_NAME));
    printf("  Backup file  : %s  (%s)\n",
           BACKUP_FILE,
           file_exists(BACKUP_FILE) ? "EXISTS" : "NOT FOUND");
    printf("  Export file  : %s  (%s)\n\n",
           EXPORT_FILE,
           file_exists(EXPORT_FILE) ? "EXISTS" : "NOT FOUND");
}
