#include "boot_time_report.h"

char hostname[128] = "";

const char* get_bootstage_id_name(int id) {
	if (id < 0 || id >= sizeof(bootstage_id_names)/sizeof(bootstage_id_names[0]) || bootstage_id_names[id] == NULL)
		return "UNKNOWN_BOOTSTAGE_ID";
	return bootstage_id_names[id];
}

void export_html(const char *filename, int count) {
	FILE *fp = fopen(filename, "w");
	if (!fp) return;

	fprintf(fp, "<html><head><script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body>");
	fprintf(fp, "<h1>%s Boot Timeline</h3><canvas id='chart'></canvas><script>", hostname);
	fprintf(fp, "const ctx=document.getElementById('chart').getContext('2d');");
	fprintf(fp, "new Chart(ctx,{type:'bar',data:{labels:[");

	for (int i = 0; i < count; i++)
		fprintf(fp, "'%s'%s", boot_records[i].name, (i < count - 1) ? "," : "");
	fprintf(fp, "],datasets:[{label:'Boot Time (ms)',data:[");

	for (int i = 0; i < count; i++)
		fprintf(fp, "%u%s", boot_records[i].start_time, (i < count - 1) ? "," : "");
	fprintf(fp, "],backgroundColor:'rgba(54,162,235,0.6)'}]},options:{indexAxis:'y'}});");
	fprintf(fp, "</script></body></html>");
	fclose(fp);
	printf("[INFO] HTML exported to %s\n", filename);
}

void print_boot_records()
{
	printf("--------------------------------------------------------------------\n");
	printf("                 %s Boot Time Report \n", hostname);
	printf("--------------------------------------------------------------------\n");

	printf("Device Power On         : %u ms\n", 0);
	printf("SPL Time		: %u ms\n", boot_summary.ustart_time);
	printf("U-BOOT Time		: %u ms\n", boot_summary.uend_time - boot_summary.ustart_time);
	printf("Kernel handoff time	: %u ms\n", boot_summary.kstart_time - boot_summary.uend_time);
	printf("Kernel Time		: %u ms\n", boot_summary.kend_time - boot_summary.kstart_time);
	printf("Total Boot Time		: %u ms\n", boot_summary.kend_time);
	printf("--------------------------------------------------------------------\n");

	for(int i = 0; i < boot_summary.count; i++)
		printf("%-30s = %6u ms (+%3u ms)\n", boot_records[i].name,
				boot_records[i].start_time,
				boot_records[i].delta_time);
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
		unsigned int time_us;
		if (sscanf(line, "%*[^I]ID:%d%*[^=]=%u", &id, &time_us) != 2) {
			continue;  // Failed to parse ID and time
		}

		unsigned int time_ms = (time_us / 1000);
		unsigned int delta_ms = (prev_time == 0) ? 0 : (time_ms - prev_time);
		strcpy(boot_records[boot_summary.count].name, get_bootstage_id_name(id));
		boot_records[boot_summary.count].start_time = time_ms;
		boot_records[boot_summary.count].delta_time = delta_ms;
		prev_time = time_ms;
		if(!index && time_us > boot_summary.uend_time)
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
		unsigned int time_ms = (records[i].start_us ? records[i].start_us : records[i].time_us) / 1000;
		boot_records[i].delta_time = (prev_time == 0) ? 0 : (time_ms - prev_time);
		boot_records[i].start_time = time_ms;
		prev_time = time_ms;

		if(rec->id == BOOTSTAGE_START_UBOOT)
			boot_summary.ustart_time = prev_time;
		if(rec->id == BOOTSTAGE_BOOTM_HANDOFF)
			boot_summary.uend_time = prev_time;
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
