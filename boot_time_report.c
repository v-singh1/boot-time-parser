#include "boot_time_report.h"

char hostname[128] = "";

const char* get_bootstage_id_name(int id) {
	if (id < 0 || id >= sizeof(bootstage_id_names)/sizeof(bootstage_id_names[0]) || bootstage_id_names[id] == NULL)
		return "UNKNOWN_BOOTSTAGE_ID";
	return bootstage_id_names[id];
}

void export_html(const char *filename, int count)
{
	FILE *fp = fopen(filename, "w");
	if (!fp) return;

	int mcu_count = boot_summary.mcu_reccount;

	/* ---- summary numbers ---- */
	unsigned dev_power_on = 0;
	unsigned spl_time     = boot_summary.ustart_time;
	unsigned uboot_time   = (boot_summary.uend_time   >= boot_summary.ustart_time)
		? (boot_summary.uend_time   - boot_summary.ustart_time) : 0;
	unsigned handoff_time = (boot_summary.kstart_time >= boot_summary.uend_time)
		? (boot_summary.kstart_time - boot_summary.uend_time)   : 0;
	unsigned kernel_time  = (boot_summary.kend_time   >= boot_summary.kstart_time)
		? (boot_summary.kend_time   - boot_summary.kstart_time) : 0;
	unsigned total_time   = boot_summary.kend_time;

	fprintf(fp,
		"<!doctype html><html><head>"
		"<meta charset='utf-8'>"
		"<meta name='viewport' content='width=device-width,initial-scale=1'>"
		"<title>%s Boot Time Report</title>"
		"<style>"
		"body{font:14px system-ui,Segoe UI,Arial;margin:16px;}"
		"h1{font-size:18px;margin:0 0 10px 0}"
		"h3{margin:18px 0 8px 0}"
		"table{border-collapse:collapse;width:100%%;font-size:12px}"
		"th,td{border:1px solid #e3e8ee;padding:6px 8px;text-align:left}"
		"th{background:#f7f9fc}"
		".row{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin:12px 0}"
		".summary{max-width:560px;margin:8px 0 16px 0}"
		".chartbox{height:720px;margin:6px 0 14px 0}"
		"</style>"
		"<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
		"</head><body>",
		hostname
	);

	fprintf(fp, "<h1>%s Boot Time Report</h1>", hostname);

	/* ---- summary table ---- */
	fprintf(fp,
		"<table class='summary'>"
		"<thead><tr><th colspan='2'>Boot Time Report Summary</th></tr></thead>"
		"<tbody>"
		"<tr><td>Device Power On</td><td>%u ms</td></tr>"
		"<tr><td>SPL Time</td><td>%u ms</td></tr>"
		"<tr><td>U-BOOT Time</td><td>%u ms</td></tr>"
		"<tr><td>Kernel handoff time</td><td>%u ms</td></tr>"
		"<tr><td>Kernel Time</td><td>%u ms</td></tr>"
		"<tr><td><b>Total Boot Time</b></td><td><b>%u ms</b></td></tr>"
		"</tbody></table>",
		dev_power_on, spl_time, uboot_time, handoff_time, kernel_time, total_time
	);

	/* ---- combined chart controls + canvas ---- */
	fprintf(fp,
		"<div class='row'>"
		" <label><input type='radio' name='mode' value='abs' checked> Absolute</label>"
		" <label><input type='radio' name='mode' value='dur'> Duration</label>"
		"</div>"
		"<div class='chartbox'><canvas id='chartCombined'></canvas></div>"
	);

	/* ---- JS arrays (Linux + MCU) ---- */
	fprintf(fp, "<script>\n");

	fprintf(fp, "const linuxN=%d, mcuN=%d;\n", count, mcu_count);

	/* labels (prefix so they’re visually grouped) */
	fprintf(fp, "const labels=[");
	for (int i = 0; i < count; ++i) {
		/* minimal escaping of single quotes for safety */
		const char *s = boot_records[i].name; char esc[256]; size_t oi=0;
		for (size_t k=0; s && s[k] && oi+2<sizeof(esc); ++k) {
			if (s[k]=='\''){esc[oi++]='\\';
				esc[oi++]='\'';
			} else
				esc[oi++]=s[k];
		}
		esc[(oi<sizeof(esc))?oi:sizeof(esc)-1]='\0';
		fprintf(fp, "'A53: %s'%s", esc, (i < count-1 || mcu_count>0) ? "," : "");
	}
	for (int i = 0; i < mcu_count; ++i) {
		const char *s = mcu_boot_records[i].name; char esc[256]; size_t oi=0;
		for (size_t k=0; s && s[k] && oi+2<sizeof(esc); ++k) {
			if (s[k]=='\''){esc[oi++]='\\';
				esc[oi++]='\'';}
			else
				esc[oi++]=s[k];
		}
		esc[(oi<sizeof(esc))?oi:sizeof(esc)-1]='\0';
		fprintf(fp, "'MCU: %s'%s", esc, (i < mcu_count-1) ? "," : "");
	}
	fprintf(fp, "];\n");

	/* Absolute values, padded with nulls in the opposite domain rows */
	fprintf(fp, "const absLinux=[");
	for (int i = 0; i < count; ++i)
		fprintf(fp, "%u,", boot_records[i].start_time);
	for (int i = 0; i < mcu_count; ++i)
		fprintf(fp, "null%s", (i < mcu_count-1) ? "," : "");
	fprintf(fp, "];\n");

	fprintf(fp, "const absMCU=[");
	for (int i = 0; i < count; ++i)
		fprintf(fp, "null,");
	for (int i = 0; i < mcu_count; ++i)
		fprintf(fp, "%u%s", mcu_boot_records[i].start_time, (i < mcu_count-1)?"," :"");
	fprintf(fp, "];\n");

	/* Deltas (used to build duration windows) */
	fprintf(fp, "const delLinux=[");
	for (int i = 0; i < count; ++i)
		fprintf(fp, "%u,", boot_records[i].delta_time);
	for (int i = 0; i < mcu_count; ++i)
		fprintf(fp, "null%s", (i < mcu_count-1) ? "," : "");
	fprintf(fp, "];\n");

	fprintf(fp, "const delMCU=[");
	for (int i = 0; i < count; ++i)
		fprintf(fp, "null,");
	for (int i = 0; i < mcu_count; ++i)
		fprintf(fp, "%u%s", mcu_boot_records[i].delta_time, (i < mcu_count-1)?"," :"");
	fprintf(fp, "];\n");

	/* ---- JS helpers + chart (Duration = [start, start+delta]) ---- */
	fprintf(fp,
		"const fmt=(v)=>v.toString()+\" ms\";\n"
		/* Build duration windows anchored to true start_time (absolute) */
		"function toWindows(absArr, delArr){"
		"  const out=[];"
		"  for(let i=0;i<absArr.length;i++){"
		"    const s=absArr[i], d=delArr[i];"
		"    if(s==null || d==null){ out.push(null); }"
		"    else{ out.push([s, s + d]); }"
		"  }"
		"  return out;"
		"}\n"
		"let durLinux = toWindows(absLinux, delLinux);\n"
		"let durMCU   = toWindows(absMCU,   delMCU);\n"

		/* Value labels plugin (works for both absolute and window bars) */
		"Chart.register({id:'valueOnBar',afterDatasetsDraw(c){"
		" const ctx=c.ctx, x=c.scales.x, y=c.scales.y; ctx.save();"
		" ctx.font='12px sans-serif'; ctx.fillStyle='#000'; ctx.textAlign='left'; ctx.textBaseline='middle';"
		" c.data.datasets.forEach(ds=>{"
		"   const D=ds.data;"
		"   D.forEach((v,i)=>{ if(v==null) return;"
		"     const val=(Array.isArray(v)?(v[1]-v[0]):v);"
		"     const xp=x.getPixelForValue(Array.isArray(v)?v[1]:v);"
		"     const yp=y.getPixelForValue(i);"
		"     ctx.fillText(fmt(val), xp+6, yp);"
		"   });"
		" });"
		" ctx.restore(); }});\n"

		/* Build combined chart with two datasets */
		"let mode='abs';\n"
		"const ctx=document.getElementById('chartCombined').getContext('2d');\n"
		"const chart=new Chart(ctx,{"
		" type:'bar',"
		" data:{"
		"   labels:labels,"
		"   datasets:["
		"     {label:'A53 / Linux', data:absLinux.slice()},"
		"     {label:'MCU',          data:absMCU.slice()}"
		"   ]"
		" },"
		" options:{indexAxis:'y',responsive:true,maintainAspectRatio:false,"
		"  scales:{x:{beginAtZero:true,title:{display:true,text:'Boot Time (ms)'}}},"
		"  plugins:{legend:{display:true},tooltip:{enabled:true},valueOnBar:{}}"
		" }});\n"

		"function render(){"
		"  chart.data.datasets[0].data = (mode==='abs') ? absLinux.slice() : durLinux.slice();"
		"  chart.data.datasets[1].data = (mode==='abs') ? absMCU.slice()   : durMCU.slice();"
		"  chart.options.parsing = (mode==='dur') ? {xAxisKey:undefined} : true;"
		"  chart.update();"
		"}\n"
		"document.querySelectorAll('input[name=\"mode\"]').forEach(r=>{"
		"  r.addEventListener('change',e=>{mode=e.target.value; render();});"
		"});\n"
		"render();\n"
	);

	/* ---- Optional tables ---- */
	fprintf(fp,
		"document.write('<h3>Bootloader & Linux Stages</h3>');\n"
		"document.write('<table><thead><tr><th>#</th><th>Stage</th><th>Absolute (ms)</th><th>Delta (ms)</th></tr></thead><tbody>');\n"
		"let idx=1; for(let i=0;i<labels.length;i++){"
		"  if(absLinux[i]==null) continue;"
		"  const name=labels[i].replace(/^A53: /,'');"
		"  document.write('<tr><td>'+ (idx++) +'</td><td>'+name+'</td><td>'+absLinux[i]+'</td><td>'+delLinux[i]+'</td></tr>');"
		"}"
		"document.write('</tbody></table>');\n"
		"if(mcuN>0){"
		"  document.write('<h3>MCU Stages</h3>');"
		"  document.write('<table><thead><tr><th>#</th><th>Stage</th><th>Absolute (ms)</th><th>Delta (ms)</th></tr></thead><tbody>');"
		"  let j=1; for(let i=0;i<labels.length;i++){"
		"    if(absMCU[i]==null) continue;"
		"    const name=labels[i].replace(/^MCU: /,'');"
		"    document.write('<tr><td>'+ (j++) +'</td><td>'+name+'</td><td>'+absMCU[i]+'</td><td>'+delMCU[i]+'</td></tr>');"
		"  }"
		"  document.write('</tbody></table>');"
		"}\n"
	);

	fprintf(fp, "</script></body></html>\n");
	fclose(fp);
}

void print_boot_records()
{
	printf("--------------------------------------------------------------------\n");
	printf("                 %s Boot Time Report \n", hostname);
	printf("--------------------------------------------------------------------\n");

	printf("Device Power On         : %u ms\n", 0);
	printf("SPL Time		: %u ms\n", boot_summary.ustart_time);
	printf("U-Boot Time		: %u ms\n", (boot_summary.uend_time - boot_summary.ustart_time));
	printf("Kernel handoff time	: %u ms\n", (boot_summary.kstart_time - boot_summary.uend_time));
	printf("Kernel Time		: %u ms\n", (boot_summary.kend_time - boot_summary.kstart_time));
	printf("Total Boot Time		: %u ms\n", boot_summary.kend_time);
	printf("--------------------------------------------------------------------\n\n");
	printf("--------------------------------------------------------------------\n");
	printf("                 Bootloader and Kernel Boot Records\n");
	printf("--------------------------------------------------------------------\n");
	for(int i = 0; i < boot_summary.count; i++)
		printf("%-30s = %6u ms (+%3u ms)\n", boot_records[i].name,
				boot_records[i].start_time,
				boot_records[i].delta_time);
	printf("--------------------------------------------------------------------\n\n");
	printf("--------------------------------------------------------------------\n");
	printf("                 MCU Boot Records \n");
	printf("--------------------------------------------------------------------\n");
	for(int i = 0; i <  boot_summary.mcu_reccount; i++)
		printf("%-30s = %6u ms (+%3u ms)\n", mcu_boot_records[i].name,
				mcu_boot_records[i].start_time,
				mcu_boot_records[i].delta_time);
	printf("--------------------------------------------------------------------\n");
}

void read_kernel_boot_records(const char* filename) {
	char line[512];
	int index = 0;
	//int cnt = boot_summary.count;

	FILE *fp = fopen(filename, "r");
	if (!fp) {
		perror("Failed to open kernel log");
		return;
	}

	while (fgets(line, sizeof(line), fp)) {
		char *tag = strstr(line, "[BOOT TRACKER]");
		if (!tag)
			continue;

		int id;
		unsigned int time;
		if (sscanf(line, "%*[^I]ID:%d%*[^=]=%u", &id, &time) != 2) {
			continue;  // Failed to parse ID and time
		}

		unsigned int time_ms = (time / 1000);
		unsigned int delta_us = (prev_time == 0) ? 0 : (time_ms - prev_time);
		strcpy(boot_records[boot_summary.count].name, get_bootstage_id_name(id));
		boot_records[boot_summary.count].start_time = time_ms;
		boot_records[boot_summary.count].delta_time = delta_us;
		prev_time = time_ms;
		if(!index && time_ms > boot_summary.uend_time)
			boot_summary.kstart_time = prev_time;
		else if(time_ms > boot_summary.kstart_time)
			boot_summary.kend_time = time_ms;
		boot_summary.count++;
		index ++;
	}
	fclose(fp);
}

int read_ubootstage_records_from_mem()
{
	int fd;
	void *mapped;
	uint8_t *buffer;

	/* Open /dev/mem for read-only access */
	fd = open("/dev/mem", O_RDONLY);
	if (fd < 0) {
		perror("Error opening /dev/mem");
		return EXIT_FAILURE;
	}
	/* Map the bootstage region into our process address space */
	mapped = mmap(NULL, BOOTSTAGE_SIZE, PROT_READ, MAP_SHARED, fd, BOOTSTAGE_PRESERVED_ADDR);
	if (mapped == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return EXIT_FAILURE;
	}
	close(fd);
	/* Allocate a local buffer and copy the mapped memory into it */
	buffer = malloc(BOOTSTAGE_SIZE);
	if (buffer == NULL) {
		perror("malloc");
		munmap(mapped, BOOTSTAGE_SIZE);
		return EXIT_FAILURE;
	}
	memcpy(buffer, mapped, BOOTSTAGE_SIZE);
	munmap(mapped, BOOTSTAGE_SIZE);

	/* Parse the header from our local buffer */
	struct uboot_bootstage_hdr *hdr = (struct uboot_bootstage_hdr *)buffer;
	if (hdr->magic != BOOTSTAGE_MAGIC || hdr->size == 0) {
		fprintf(stderr, "Invalid bootstage header: magic=0x%08x, size=0x%x\n",
		hdr->magic, hdr->size);
		free(buffer);
		return EXIT_FAILURE;
	}
#ifdef DEBUG
	printf(" Version : %u\n", hdr->version);
	printf(" Count : %u\n", hdr->count);
	printf(" Size : 0x%x\n", hdr->size);
	printf(" Magic : 0x%08x\n", hdr->magic);
	printf(" Next ID : %u\n", hdr->next_id);
#endif
	boot_summary.count = hdr->count;
	/* The bootstage records follow immediately after the header */
	struct uboot_bootstage_record *records =
		(struct uboot_bootstage_record *)(buffer + sizeof(struct uboot_bootstage_hdr));

	/* Loop through and print each record (limit to the lower of hdr->count and RECORD_COUNT) */
	for (int i = 0; i < (int)hdr->count && i < RECORD_COUNT; i++) {
		struct uboot_bootstage_record *rec = &records[i];
		strcpy(boot_records[i].name, get_bootstage_id_name(rec->id));
		uint64_t time_ms = ((records[i].start_us ? records[i].start_us : records[i].time_us) / 1000);
		boot_records[i].delta_time = (prev_time == 0) ? 0 : (time_ms - prev_time);
		boot_records[i].start_time = time_ms;
		prev_time = time_ms;

		if(rec->id == BOOTSTAGE_START_UBOOT)
			boot_summary.ustart_time = prev_time;
		if(rec->id == BOOTSTAGE_BOOTM_HANDOFF)
			boot_summary.uend_time = prev_time;
		if(rec->id == BOOTSTAGE_START_MCU)
			 boot_summary.mcu_start_time = prev_time;
	}

	/* Other subsystem (MCU/DSP) boot record parsing */
	mcu_boot_stage_record_t *mcuhdr = (mcu_boot_stage_record_t *)(buffer + MCU_BOOTSTAGE_START_OFFSET);
	boot_summary.mcu_reccount = mcuhdr -> record_count + 1;

#ifdef DEBUG
	printf("Subsystem(MCU) record id = %x\n", mcuhdr -> record_id);
	printf("MCU:%d record count = %d\n", mcuhdr -> record_id, mcuhdr -> record_count);
	printf("MCU:%d record start time = %llu\n", mcuhdr -> record_id, mcuhdr -> start_time);
#endif
	uint64_t mcu_prev_time = boot_summary.mcu_start_time;
	strcpy(mcu_boot_records[0].name, "MCU_AWAKE");
	mcu_boot_records[0].start_time = boot_summary.mcu_start_time;
	mcu_boot_records[0].delta_time = 0;

	mcu_boot_record_profile_t *rec =  (mcu_boot_record_profile_t *)(buffer + MCU_BOOTSTAGE_START_OFFSET + MCU_BOOTRECORD_OFFSET);
	for (int i = 0; i < (int)mcuhdr -> record_count; i++) {
		mcu_boot_record_profile_t * record = &rec[i];
		strcpy(mcu_boot_records[i+1].name, record -> name);
		mcu_boot_records[i+1].start_time = (record -> time / 1000 + boot_summary.mcu_start_time);
		mcu_boot_records[i+1].delta_time = (mcu_prev_time == 0) ? 0 : (mcu_boot_records[i+1].start_time - mcu_prev_time);
		mcu_prev_time = mcu_boot_records[i+1].start_time;
	}

	free(buffer);
}

int main(void)
{
	if(gethostname(hostname, sizeof(hostname)) != 0)
		perror("gethostname failed\n");

	read_ubootstage_records_from_mem();
	read_kernel_boot_records("/var/log/messages");
	print_boot_records();
	export_html("boot_time_report.html",  boot_summary.count);
	return EXIT_SUCCESS;
}
