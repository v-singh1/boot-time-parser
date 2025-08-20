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

+--------------------------------------------------------------------+
                 am62xx-evm Boot Time Report 
+--------------------------------------------------------------------+
Device Power On         : 0 ms
SPL Time                : 843 ms
U-Boot Time             : 2173 ms
Kernel handoff time     : 462 ms
Kernel Time             : 2522 ms
Total Boot Time         : 6000 ms
+--------------------------------------------------------------------+

+--------------------------------------------------------------------+
                 Bootloader and Kernel Boot Records
+--------------------------------------------------------------------+
BOOTSTAGE_AWAKE                =      0 ms (+  0 ms)
BOOTSTAGE_START_UBOOT_F        =    843 ms (+  0 ms)
BOOTSTAGE_ACCUM_DM_F           =    843 ms (+  0 ms)
BOOTSTAGE_START_UBOOT_R        =   1951 ms (+1108 ms)
BOOTSTAGE_ACCUM_DM_R           =   1951 ms (+  0 ms)
BOOTSTAGE_NET_ETH_START        =   2032 ms (+ 81 ms)
BOOTSTAGE_NET_ETH_INIT         =   2053 ms (+ 21 ms)
BOOTSTAGE_MAIN_LOOP            =   2055 ms (+  2 ms)
BOOTSTAGE_START_MCU            =   2661 ms (+606 ms)
BOOTSTAGE_BOOTM_START          =   2959 ms (+298 ms)
BOOTSTAGE_RUN_OS               =   3016 ms (+ 57 ms)
BOOTSTAGE_BOOTM_HANDOFF        =   3016 ms (+  0 ms)
BOOTSTAGE_KERNEL_START         =   3478 ms (+462 ms)
BOOTSTAGE_KERNEL_END           =   6000 ms (+2522 ms)
+--------------------------------------------------------------------+

+--------------------------------------------------------------------+
                 MCU Boot Records 
+--------------------------------------------------------------------+
<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>am62xx-evm Boot Time Report</title><style>body{font:14px system-ui,Segoe UI,Arial;margin:16px;}h1{font-size:18px;margin:0 0 10px 0}h3{margin:18px 0 8px 0}table{border-collapse:collapse;width:100%;font-size:12px}th,td{border:1px solid #e3e8ee;padding:6px 8px;text-align:left}th{background:#f7f9fc}.row{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin:12px 0}.summary{max-width:560px;margin:8px 0 16px 0}.chartbox{height:720px;margin:6px 0 14px 0}</style><script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body><h1>am62xx-evm Boot Time Report</h1><table class='summary'><thead><tr><th colspan='2'>Boot Time Report Summary</th></tr></thead><tbody><tr><td>Device Power On</td><td>0 ms</td></tr><tr><td>SPL Time</td><td>819 ms</td></tr><tr><td>U-BOOT Time</td><td>2138 ms</td></tr><tr><td>Kernel handoff time</td><td>463 ms</td></tr><tr><td>Kernel Time</td><td>2923 ms</td></tr><tr><td><b>Total Boot Time</b></td><td><b>6343 ms</b></td></tr></tbody></table><div class='row'> <label><input type='radio' name='mode' value='abs' checked> Absolute</label> <label><input type='radio' name='mode' value='dur'> Duration</label></div><div class='chartbox'><canvas id='chartCombined'></canvas></div><script>
function esc(s){return (s||'').replaceAll("'","\\'");}
const linuxN=14, mcuN=11;
const labels=['A53: BOOTSTAGE_AWAKE','A53: BOOTSTAGE_START_UBOOT_F','A53: BOOTSTAGE_ACCUM_DM_F','A53: BOOTSTAGE_START_UBOOT_R','A53: BOOTSTAGE_ACCUM_DM_R','A53: BOOTSTAGE_NET_ETH_START','A53: BOOTSTAGE_NET_ETH_INIT','A53: BOOTSTAGE_MAIN_LOOP','A53: BOOTSTAGE_START_MCU','A53: BOOTSTAGE_BOOTM_START','A53: BOOTSTAGE_RUN_OS','A53: BOOTSTAGE_BOOTM_HANDOFF','A53: BOOTSTAGE_KERNEL_START','A53: BOOTSTAGE_KERNEL_END','MCU: MCU_AWAKE','MCU: BOARD_PERIPHERALS_INIT','MCU: MAIN_TASK_CREATE','MCU: FIRST_TASK','MCU: DRIVERS_OPEN','MCU: BOARD_DRIVERS_OPEN','MCU: IPC_SYNC_FOR_LINUX','MCU: IPC_REGISTER_CLIENT','MCU: IPC_SUSPEND_TASK','MCU: IPC_RECEIVE_TASK','MCU: IPC_SYNC_ALL'];
const absLinux=[0,819,819,1930,1930,2011,2032,2035,2625,2901,2957,2957,3420,6343,null,null,null,null,null,null,null,null,null,null,null];
const absMCU=[null,null,null,null,null,null,null,null,null,null,null,null,null,null,2625,2625,2625,2626,2626,2626,6945,6945,6945,6945,7092];
const delLinux=[0,0,0,1111,0,81,21,3,590,276,56,0,463,2923,null,null,null,null,null,null,null,null,null,null,null];
const delMCU=[null,null,null,null,null,null,null,null,null,null,null,null,null,null,0,0,0,1,0,0,4319,0,0,0,147];
const fmt=(v)=>v.toString()+" ms";
function buildDurFromDeltas(d){  const out=[]; let acc=0;  for(let i=0;i<d.length;i++){    if(d[i]==null){ out.push(null); }    else{ let s=acc; acc+=d[i]; out.push([s,acc]); }  } return out;}
let durLinux=buildDurFromDeltas(delLinux);
let durMCU=buildDurFromDeltas(delMCU);
Chart.register({id:'valueOnBar',afterDatasetsDraw(c){ const ctx=c.ctx, x=c.scales.x, y=c.scales.y; ctx.save(); ctx.font='12px sans-serif'; ctx.fillStyle='#000'; ctx.textAlign='left'; ctx.textBaseline='middle'; c.data.datasets.forEach(ds=>{   const D=ds.data;   D.forEach((v,i)=>{ if(v==null) return;     const val=(Array.isArray(v)?(v[1]-v[0]):v);     const xp=x.getPixelForValue(Array.isArray(v)?v[1]:v);     const yp=y.getPixelForValue(i);     ctx.fillText(fmt(val), xp+6, yp);   }); }); ctx.restore(); }});
let mode='abs';
const ctx=document.getElementById('chartCombined').getContext('2d');
const chart=new Chart(ctx,{ type:'bar', data:{   labels:labels,   datasets:[     {label:'A53 / Linux', data:absLinux.slice()},     {label:'MCU',          data:absMCU.slice()}   ] }, options:{indexAxis:'y',responsive:true,maintainAspectRatio:false,  scales:{x:{beginAtZero:true,title:{display:true,text:'Boot Time (ms)'}}},  plugins:{legend:{display:true},tooltip:{enabled:true},valueOnBar:{}} }});
function render(){  chart.data.datasets[0].data = (mode==='abs') ? absLinux.slice() : durLinux.slice();  chart.data.datasets[1].data = (mode==='abs') ? absMCU.slice()   : durMCU.slice();  chart.options.parsing = (mode==='dur') ? {xAxisKey:undefined} : true;  chart.update();}
document.querySelectorAll('input[name="mode"]').forEach(r=>{  r.addEventListener('change',e=>{mode=e.target.value; render();});});
render();
document.write('<h3>Linux Stages</h3>');
document.write('<table><thead><tr><th>#</th><th>Stage</th><th>Absolute (ms)</th><th>Delta (ms)</th></tr></thead><tbody>');
let idx=1; for(let i=0;i<labels.length;i++){  if(absLinux[i]==null) continue;  const name=labels[i].replace(/^A53: /,'');  document.write('<tr><td>'+ (idx++) +'</td><td>'+name+'</td><td>'+absLinux[i]+'</td><td>'+delLinux[i]+'</td></tr>');}document.write('</tbody></table>');
if(mcuN>0){  document.write('<h3>MCU Stages</h3>');  document.write('<table><thead><tr><th>#</th><th>Stage</th><th>Absolute (ms)</th><th>Delta (ms)</th></tr></thead><tbody>');  let j=1; for(let i=0;i<labels.length;i++){    if(absMCU[i]==null) continue;    const name=labels[i].replace(/^MCU: /,'');    document.write('<tr><td>'+ (j++) +'</td><td>'+name+'</td><td>'+absMCU[i]+'</td><td>'+delMCU[i]+'</td></tr>');  }  document.write('</tbody></table>');}
</script></body></html>
MCU_AWAKE                      =   2661 ms (+  0 ms)
BOARD_PERIPHERALS_INIT         =   2661 ms (+  0 ms)
MAIN_TASK_CREATE               =   2661 ms (+  0 ms)
FIRST_TASK                     =   2662 ms (+  1 ms)
DRIVERS_OPEN                   =   2662 ms (+  0 ms)
BOARD_DRIVERS_OPEN             =   2662 ms (+  0 ms)
IPC_SYNC_FOR_LINUX             =   6636 ms (+3974 ms)
IPC_REGISTER_CLIENT            =   6636 ms (+  0 ms)
IPC_SUSPEND_TASK               =   6636 ms (+  0 ms)
IPC_RECEIVE_TASK               =   6636 ms (+  0 ms)
IPC_SYNC_ALL                   =   6787 ms (+151 ms)
+--------------------------------------------------------------------+

Graphical output
[boot_time_report_v3.html](https://github.com/user-attachments/files/21901381/boot_time_report_v3.html)



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

