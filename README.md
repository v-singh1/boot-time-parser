# Boot Time Report Framework

This project provides a unified boot time measurement framework for embedded platforms with heterogeneous compute architecture (e.g., ARM + MCU + DSP). It captures precise boot stage timestamps from SPL, U-Boot, Kernel, and User-space, enabling developers to analyze and optimize end-to-end boot latency.

## ✨ Key Features

- Architecture-agnostic design (supports ARM64, x86, MIPS, etc.)
- Hardware counter–based timestamping (e.g., CNTVCT_EL0 on ARM64)
- Common Linux kernel API: `boot_time_now()` for nanosecond timestamps
- Boot stage logging for:
  - SPL
  - U-Boot
  - Kernel start/init
  - MCU firmware
  - First user-space app
- Lightweight C-based parser to generate boot report
- Output report in human-readable and graphical format

## ⚙️ Build & Install

```bash
mkdir build
cd build
cmake ..
make
sudo make install

📊 Sample Boot Report

+-----------------------------------------------------+
                 am62xx-evm Boot Time Report
+-----------------------------------------------------+
Device Power On         : 0 ms
SPL Time                : 1902 ms
U-BOOT Time             : 3007 ms
Kernel handoff time     : 541 ms
Kernel Time             : 2892 ms
Total Boot Time         : 8342 ms
+-----------------------------------------------------+
BOOTSTAGE_ID_AWAKE             =      0 ms (+  0 ms)
BOOTSTAGE_ID_START_UBOOT_F     =    797 ms (+  0 ms)
BOOTSTAGE_ID_ACCUM_DM_F        =    797 ms (+  0 ms)
BOOTSTAGE_ID_START_UBOOT_R     =   1902 ms (+1105 ms)
BOOTSTAGE_ID_ACCUM_DM_R        =   1902 ms (+  0 ms)
BOOTSTAGE_ID_NET_ETH_START     =   1982 ms (+ 80 ms)
BOOTSTAGE_ID_NET_ETH_INIT      =   2004 ms (+ 22 ms)
BOOTSTAGE_ID_MAIN_LOOP         =   2006 ms (+  2 ms)
BOOTSTAGE_ID_BOOTM_START       =   4853 ms (+2847 ms)
BOOTSTAGE_ID_RUN_OS            =   4909 ms (+ 56 ms)
BOOTSTAGE_ID_BOOTM_HANDOFF     =   4909 ms (+  0 ms)
BOOTSTAGE_ID_KERNEL_START      =   5450 ms (+541 ms)
BOOTSTAGE_ID_KERNEL_END        =   8342 ms (+2892 ms)
+-----------------------------------------------------+

📌 Usage

1. Apply kernel and U-Boot patches from kernel_patch/ and uboot_patch/.


2. Build and install the user-space utility from user_app/.


3. Run the tool post-boot:

sudo boot_time_report_parser


4. Optional: Use utils/gen_graph.py to generate a timeline graph.



🛠 Platforms Tested

TI Sitara AM62x (Linux SDK 11.0+)

ARM Cortex-A53 with Cortex-M4 MCU

U-Boot 2024.x

Linux Kernel 6.12+


📄 License

MIT License (or BSD-3-Clause if aligned with TI SDK licensing)

---

🤝 Contributing

We welcome contributions! Please follow standard GitHub practices and submit a pull request. Make sure to run clang-format and check coding style before submitting.

🔗 References

TI Linux SDK Documentation

U-Boot Bootstage

Linux Kernel Timing APIs

