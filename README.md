# 💸 PayFlow — Payroll Dashboard

> A modern, fully client-side payroll management system for small teams and HR managers. No backend, no database, no server — just open and run.
> 
🌐 **Live Demo:** [kunal0713.github.io/PayRoll_System](https://kunal0713.github.io/PayRoll_System/)

---

## ✨ What is PayFlow?

**PayFlow** is a single-page payroll dashboard that lets you manage your entire team's salary workflow from one clean interface. It handles everything from adding employees and calculating taxes to generating professional PDF payslips and exporting monthly reports — all running entirely in the browser.

The core payroll engine is written in **C** (`payroll_engine.c`) and faithfully ported to **vanilla JavaScript** for the browser. There is no framework, no backend API, and no cloud dependency. Data is stored locally in the browser using `localStorage`.

---

## 🖥️ Features

| Feature | Details |
|---|---|
| 👥 Employee Management | Add, search, and delete employees with real-time validation |
| 💰 Auto Payroll Calculation | Gross pay, overtime, tax brackets, and net pay computed instantly |
| 📄 PDF Payslips | Generate and download professional payslips as real `.pdf` files |
| 📊 Analytics Dashboard | Bar and line charts powered by Chart.js with live payroll counter |
| 📑 Monthly Reports | Export full payroll report as PDF or CSV with one click |
| 🌗 Dark / Light Mode | Full theme support with smooth transitions |
| 🔔 Notifications | In-app alerts for key payroll events |
| 💾 Offline-first | Works completely offline — no internet required after first load |

---

## 🧮 Payroll Logic

Tax is calculated on **gross pay** (basic salary + overtime) using these brackets:

| Gross Pay | Tax Rate |
|---|---|
| Below ₹30,000 | 5% |
| ₹30,000 – ₹59,999 | 10% |
| ₹60,000 and above | 15% |

**Overtime rate:** ₹150 per hour

```
Net Pay = (Basic Pay + OT Hours × ₹150) − Income Tax
```

---

## 🚀 Quick Start

**Requirements:** Node.js 18+ and npm 9+

```bash
# 1. Clone the repo
git clone https://github.com/Kunal0713/PayRoll_System.git
cd PayRoll_System

# 2. Install dependencies
npm install

# 3. Start dev server — opens at http://localhost:3000
npm run dev

# 4. Build for production
npm run build
```

---

## 🗂️ Project Structure

```
payflow/
├── public/
│   └── index.html          ← Full PayFlow web app (HTML + CSS + JS)
├── src/
│   └── utils.js            ← Shared JS utilities (formatting, tax, CSV, storage)
├── payroll_engine.c         ← Core payroll engine written in C
├── package.json
├── vite.config.js
└── README.md
```

---

## ⚙️ C Engine

The payroll calculation logic is also implemented in pure C for performance and portability. The browser JavaScript is a direct port of this engine.

```bash
# Compile
gcc -o payroll_engine payroll_engine.c -lm

# Run — prints a full payroll report + sample payslip
./payroll_engine
```

The C engine supports: employee CRUD, tax calculation, gross/net pay, department management, sorting, payslip printing, and summary reports with colour terminal output.

---

## 🛠️ Tech Stack

- **Frontend:** Vanilla HTML, CSS, JavaScript (no framework)
- **Charts:** Chart.js 4.4
- **PDF Engine:** Custom pure-JS PDF byte-stream writer (no library)
- **Build Tool:** Vite 5
- **Core Logic:** C (ported to JS for the browser)
- **Storage:** Browser localStorage

---

## 📸 Pages

- **Dashboard** — Stat cards, payroll charts, quick actions, insights
- **Employees** — Full employee table with search and delete
- **Generate Payslip** — Look up any employee and download their PDF payslip
- **Reports** — Monthly payroll PDF, salary report PDF, and CSV export

---

## 📄 License

MIT — free to use, modify, and distribute.
