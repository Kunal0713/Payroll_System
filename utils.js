/**
 * utils.js — PayFlow Shared Utilities & State Management
 * ─────────────────────────────────────────────────────
 * Formatting, payroll math, sorting, CSV export helpers,
 * and local-storage persistence used across the dashboard.
 * Mirrors the logic in payroll_engine.c
 */

/* ── Constants ─────────────────────────────────────────────────────────────── */
export const OT_RATE         = 150;
export const TAX_BRACKET_LOW  = 0.05;
export const TAX_BRACKET_MID  = 0.10;
export const TAX_BRACKET_HIGH = 0.15;
export const THRESHOLD_LOW    = 30000;
export const THRESHOLD_HIGH   = 60000;
export const STORAGE_KEY      = 'payflow_db';
export const VERSION          = '1.0.0';

/* ── Currency formatting ────────────────────────────────────────────────────── */

/**
 * Format a number as Indian Rupee string.
 * e.g. 45000 → "₹45,000.00"
 * @param {number} amount
 * @returns {string}
 */
export function formatINR(amount) {
  return '₹' + Number(amount)
    .toFixed(2)
    .replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

/**
 * Format as compact INR (no decimals for whole numbers).
 * e.g. 45000 → "₹45,000"
 */
export function formatINRCompact(amount) {
  const n = Number(amount);
  return '₹' + (Number.isInteger(n) ? n : n.toFixed(2))
    .toString()
    .replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

/* ── Tax helpers ────────────────────────────────────────────────────────────── */

/**
 * Calculate income tax based on gross pay.
 * < 30,000  →  5%
 * < 60,000  → 10%
 * >= 60,000 → 15%
 * @param {number} grossPay
 * @returns {number} tax amount
 */
export function calcTax(grossPay) {
  if (grossPay < THRESHOLD_LOW)  return grossPay * TAX_BRACKET_LOW;
  if (grossPay < THRESHOLD_HIGH) return grossPay * TAX_BRACKET_MID;
  return grossPay * TAX_BRACKET_HIGH;
}

/**
 * Return the tax bracket percentage label for a gross pay value.
 * @param {number} grossPay
 * @returns {string} e.g. "10%"
 */
export function taxBracketLabel(grossPay) {
  if (grossPay < THRESHOLD_LOW)  return '5%';
  if (grossPay < THRESHOLD_HIGH) return '10%';
  return '15%';
}

/* ── Core payroll calculation ───────────────────────────────────────────────── */

/**
 * Compute gross pay, tax, and net pay for a given employee.
 * Mirrors _calc() in payroll_engine.c
 * @param {{ basicPay: number, otHours: number }} emp
 * @returns {{ grossPay: number, tax: number, netPay: number }}
 */
export function calcEmployee(emp) {
  const grossPay = emp.basicPay + emp.otHours * OT_RATE;
  const tax      = calcTax(grossPay);
  const netPay   = grossPay - tax;
  return { grossPay, tax, netPay };
}

/**
 * Mutate an employee object in-place with calculated fields.
 * @param {object} emp
 */
export function applyCalc(emp) {
  const result = calcEmployee(emp);
  Object.assign(emp, result);
}

/* ── Aggregates ─────────────────────────────────────────────────────────────── */

/** @param {object[]} employees */
export function totalNetPayroll(employees) {
  return employees.reduce((sum, e) => sum + (e.netPay || 0), 0);
}

export function totalGrossPayroll(employees) {
  return employees.reduce((sum, e) => sum + (e.grossPay || 0), 0);
}

export function totalTaxCollected(employees) {
  return employees.reduce((sum, e) => sum + (e.tax || 0), 0);
}

export function averageSalary(employees) {
  if (!employees.length) return 0;
  return totalNetPayroll(employees) / employees.length;
}

export function highestEarner(employees) {
  if (!employees.length) return null;
  return employees.reduce((hi, e) => e.netPay > hi.netPay ? e : hi, employees[0]);
}

export function lowestEarner(employees) {
  if (!employees.length) return null;
  return employees.reduce((lo, e) => e.netPay < lo.netPay ? e : lo, employees[0]);
}

/**
 * Full summary stats for a list of employees.
 * @param {object[]} employees
 * @returns {object}
 */
export function computeSummary(employees) {
  return {
    count:       employees.length,
    totalGross:  totalGrossPayroll(employees),
    totalTax:    totalTaxCollected(employees),
    totalNet:    totalNetPayroll(employees),
    averageNet:  averageSalary(employees),
    highest:     highestEarner(employees),
    lowest:      lowestEarner(employees),
  };
}

/* ── Sorting ────────────────────────────────────────────────────────────────── */

/** @returns {object[]} new sorted array, does not mutate */
export function sortByNetPay(employees, ascending = false) {
  return [...employees].sort((a, b) =>
    ascending ? a.netPay - b.netPay : b.netPay - a.netPay
  );
}

export function sortByName(employees) {
  return [...employees].sort((a, b) => a.name.localeCompare(b.name));
}

export function sortById(employees) {
  return [...employees].sort((a, b) => a.empID - b.empID);
}

export function sortByBasicPay(employees, ascending = false) {
  return [...employees].sort((a, b) =>
    ascending ? a.basicPay - b.basicPay : b.basicPay - a.basicPay
  );
}

/* ── Filtering ──────────────────────────────────────────────────────────────── */

/**
 * Filter employees by a search string (name or ID).
 * @param {object[]} employees
 * @param {string} query
 * @returns {object[]}
 */
export function filterEmployees(employees, query) {
  if (!query) return employees;
  const q = query.toLowerCase();
  return employees.filter(e =>
    e.name.toLowerCase().includes(q) ||
    String(e.empID).includes(q)
  );
}

/* ── Validation ─────────────────────────────────────────────────────────────── */

/**
 * Validate employee fields before adding to DB.
 * @returns {{ valid: boolean, error: string }}
 */
export function validateEmployee(id, name, basicPay, otHours) {
  if (!id || isNaN(id) || id <= 0)
    return { valid: false, error: 'Employee ID must be a positive number.' };
  if (!name || name.trim().length < 2)
    return { valid: false, error: 'Name must be at least 2 characters.' };
  if (isNaN(basicPay) || basicPay < 0)
    return { valid: false, error: 'Basic pay must be a non-negative number.' };
  if (isNaN(otHours) || otHours < 0)
    return { valid: false, error: 'OT hours must be a non-negative number.' };
  return { valid: true, error: '' };
}

/* ── CSV export ─────────────────────────────────────────────────────────────── */

/**
 * Generate a CSV string from an array of employee objects.
 * @param {object[]} employees
 * @returns {string}
 */
export function toCSV(employees) {
  const headers = ['ID', 'Name', 'Department', 'Basic Pay', 'OT Hours',
                   'Gross Pay', 'Tax', 'Net Pay', 'Tax Bracket'];
  const rows = employees.map(e => [
    e.empID,
    `"${String(e.name).replace(/"/g, '""')}"`,
    'General',
    e.basicPay.toFixed(2),
    e.otHours,
    e.grossPay.toFixed(2),
    e.tax.toFixed(2),
    e.netPay.toFixed(2),
    taxBracketLabel(e.grossPay),
  ]);
  return [headers, ...rows].map(r => r.join(',')).join('\n');
}

/**
 * Trigger a CSV file download in the browser.
 * @param {string} csv
 * @param {string} filename
 */
export function downloadCSV(csv, filename = 'payroll.csv') {
  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
  const url  = URL.createObjectURL(blob);
  const a    = document.createElement('a');
  a.href     = url;
  a.download = filename;
  a.style.display = 'none';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 5000);
}

/* ── LocalStorage persistence ───────────────────────────────────────────────── */

/**
 * Save employee array to localStorage.
 * @param {object[]} employees
 */
export function saveToStorage(employees) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(employees));
    return true;
  } catch (e) {
    console.error('PayFlow: failed to save to localStorage', e);
    return false;
  }
}

/**
 * Load employee array from localStorage.
 * @returns {object[]}
 */
export function loadFromStorage() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch (e) {
    console.error('PayFlow: failed to load from localStorage', e);
    return [];
  }
}

/** Clear all stored payroll data. */
export function clearStorage() {
  try { localStorage.removeItem(STORAGE_KEY); } catch (_) {}
}

/* ── Date helpers ───────────────────────────────────────────────────────────── */

/** @returns {string} e.g. "March 2026" */
export function currentPeriod() {
  return new Date().toLocaleString('en-IN', { month: 'long', year: 'numeric' });
}

/** @returns {string} e.g. "22/3/2026" */
export function currentDate() {
  return new Date().toLocaleDateString('en-IN');
}

/* ── Misc ───────────────────────────────────────────────────────────────────── */

/**
 * Generate initials from a full name.
 * "Aarav Sharma" → "AS"
 */
export function initials(name) {
  return name.trim().split(/\s+/)
    .map(w => w[0].toUpperCase())
    .slice(0, 2)
    .join('');
}

/**
 * Clamp a number between min and max.
 */
export function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}
