#include "monitor.h"
#include <sys/stat.h>  

/**
 * Helper: reads the /proc/[pid]/stat file for a given process.
 *
 * @param pid  Process ID.
 * @param buf  Output buffer for the stat line.
 * @param size Size of the output buffer.
 * @return     0 on success, -1 on failure.
 *
 * @sideeffect Opens and reads the process stat file.
 */
static int read_proc_stat(pid_t pid, char *buf, size_t size) 

{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    if (!fgets(buf, size, fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

/**
 * Extracts fields from /proc/[pid]/stat format.
 * Parses process name, user time, system time, and RSS.
 *
 * @param buf       Input buffer containing the stat line.
 * @param comm      Output buffer for process name.
 * @param comm_size Size of comm buffer.
 * @param utime     Output for user time (jiffies).
 * @param stime     Output for system time (jiffies).
 * @param rss       Output for resident set size (pages).
 * @return          0 on success, -1 on parse error.
 *
 * @note Format: pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime ...
 */
static int parse_stat(const char *buf, char *comm, size_t comm_size,
                      unsigned long long *utime, unsigned long long *stime,
                      long *rss) {
    /* Format: pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss rsslim ... */
    const char *p = buf;
    /* skip pid */
    while (*p && *p != ' ') p++;
    while (*p && *p == ' ') p++;
    if (*p != '(') return -1;
    p++; /* skip '(' */
    const char *comm_start = p;
    while (*p && *p != ')') p++;
    if (*p != ')') return -1;
    size_t len = p - comm_start;
    if (len >= comm_size) len = comm_size - 1;
    strncpy(comm, comm_start, len);
    comm[len] = '\0';
    p++; /* skip ')' */
    /* skip to field 14 (utime) - fields counted from 1 (pid is field 1) */
    int field = 3; /* consumed pid and comm, next is state */
    while (field < 14 && *p) {
        while (*p && *p == ' ') p++;
        while (*p && *p != ' ') p++;
        field++;
    }
    if (field < 14) return -1;
    if (sscanf(p, "%llu", utime) != 1) return -1;
    /* skip to stime */
    while (*p && *p != ' ') p++;
    while (*p && *p == ' ') p++;
    if (sscanf(p, "%llu", stime) != 1) return -1;
    /* skip to field 24 (rss) - field 16 is starttime, need to advance to field 24 */
    for (int i = 16; i <= 23; i++) {
        while (*p && *p != ' ') p++;
        while (*p && *p == ' ') p++;
    }
    if (sscanf(p, "%ld", rss) != 1) return -1;
    return 0;
}

/**
 * Checks if a given path is a directory.
 * Uses stat() for portability (avoids d_type issues).
 *
 * @param path Path to check.
 * @return 1 if directory, 0 otherwise.
 */
static int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

/**
 * Reads total system memory from /proc/meminfo.
 * Caches the result after first read.
 *
 * @return Total memory in bytes, or 0 on failure.
 */
static guint64 get_total_memory(void) {
    static guint64 total = 0;
    if (total != 0) return total;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &total) == 1) break;
    }
    fclose(fp);
    return total;
}

/**
 * Retrieves a list of running processes with memory usage.
 * Reads from /proc directory and parses process statistics.
 *
 * @return GList containing ProcessEntry structures.
 *         Caller must free with free_process_list().
 *
 * @sideeffect Opens and reads /proc directory and stat files.
 */
GList* get_process_list(void) {
    GList *list = NULL;
    DIR *dir = opendir("/proc");
    if (!dir) return NULL;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip non-numeric entries */
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        /* Build full path and check if it's a directory */
        char path[256];
        snprintf(path, sizeof(path), "/proc/%s", entry->d_name);
        if (!is_directory(path)) continue;

        char stat_buf[1024];
        if (read_proc_stat(pid, stat_buf, sizeof(stat_buf)) != 0) continue;

        char comm[256];
        unsigned long long utime, stime;
        long rss_pages;
        if (parse_stat(stat_buf, comm, sizeof(comm), &utime, &stime, &rss_pages) != 0) continue;

        ProcessEntry *p = g_new(ProcessEntry, 1);
        p->pid = pid;
        strncpy(p->name, comm, sizeof(p->name)-1);
        p->name[sizeof(p->name)-1] = '\0';
        p->cpu_percent = 0.0; /* Placeholder – full CPU% calculation would require history */

        /* Memory percent: rss (pages) * page size / total memory */
        long page_size = sysconf(_SC_PAGESIZE);
        guint64 total_mem = get_total_memory() * 1024; /* convert kB to bytes */
        if (total_mem > 0) {
            p->mem_percent = 100.0 * (rss_pages * page_size) / total_mem;
        } else {
            p->mem_percent = 0;
        }

        list = g_list_append(list, p);
    }
    closedir(dir);
    return list;
}

/**
 * Frees a GList of ProcessEntry structures.
 *
 * @param list GList of ProcessEntry pointers to free.
 */
void free_process_list(GList *list) {
    for (GList *l = list; l != NULL; l = l->next) {
        g_free(l->data);
    }
    g_list_free(list);
}