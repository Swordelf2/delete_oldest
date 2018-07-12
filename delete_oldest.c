#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>
#include <errno.h>

const char HELP_STR[] = "delete_oldest deletes all files in given directory with given prefix and suffix except for PRES_COUNT most recent ones\n";
const char USAGE_STR[] = "Usage: delete_oldest DIR PREFIX SUFFIX [DAYS]\n";

enum Args
{
    ARG_CNT = 4 + 1,
    ARG_DIR = 1,
    ARG_PREFIX,
    ARG_SUFFIX,
    ARG_PRES_COUNT
};

enum
{
    MAX_FILE_CNT = 512,
    MAX_ARG_SIZE = 1024,
    MAX_ERROR_STR_SIZE = 1024,
};

struct DFile
{
    char *name;
    time_t cr_time;
};

int delete_oldest(char *dir_name, char *prefix, char *suffix, int days);
inline int delete_file(char *full_name, char *name);

int compare_times(const void *dfile1, const void *dfile2);

int main(int argc, char *argv[])
{
    if (argc >= 2 &&
            (strncmp(argv[1], "-h", MAX_ARG_SIZE) == 0 || strncmp(argv[1], "--help", MAX_ARG_SIZE) == 0)) {
        fputs(HELP_STR, stdout);
        fputs(USAGE_STR, stdout);
        // provide help info
    } else if (argc >= ARG_CNT) {
        int pres_count = strtol(argv[ARG_PRES_COUNT], NULL, 0);
        return delete_oldest(argv[ARG_DIR], argv[ARG_PREFIX], argv[ARG_SUFFIX], pres_count);
    } else {
        fputs(USAGE_STR, stdout);
        return 1;
    }
}

int delete_oldest(char *dir_name, char *prefix, char *suffix, int pres_count)
{
    size_t pref_len = strlen(prefix);
    size_t suf_len = strlen(suffix);
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        fprintf(stderr, "Could not open directory %s:\n%s\n", dir_name, strerror(errno));
        return -1;
    }

    struct DFile df_arr[MAX_FILE_CNT];
    size_t df_arr_size = 0;

    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir))) {
        char file_name[PATH_MAX];
        char *name = dir_ent->d_name;
        size_t name_len = strlen(name);
        // check prefix and suffix
        if (strncmp(name, prefix, pref_len) == 0 &&
            (name_len >= suf_len && strncmp(name + name_len - suf_len, suffix, suf_len) ==0)) {
            // now checkwhether it's a regular file
            snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, name);
            struct stat file_stat;
            if (lstat(file_name, &file_stat) != 0) {
                fprintf(stderr, "Could not stat file %s:\n%s\n", name, strerror(errno));
                return -1;
            }

            if (S_ISREG(file_stat.st_mode)) {
                if (df_arr_size >= MAX_FILE_CNT) {
                    fprintf(stderr, "Too many files, aborting");
                    return -1;
                }
                df_arr[df_arr_size].name = strdup(name);
                df_arr[df_arr_size].cr_time = file_stat.st_mtime;
                ++df_arr_size;
            }
        }
    }

    qsort(df_arr, df_arr_size, sizeof(*df_arr), compare_times);
    for (unsigned i = pres_count; i < df_arr_size; ++i) {
        char file_name[PATH_MAX];
        snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, df_arr[i].name);
        delete_file(file_name, df_arr[i].name);
    }

    for (size_t i = 0; i < df_arr_size; ++i) {
        free(df_arr[i].name);
    }

    if (closedir(dir) != 0) {
        fprintf(stderr, "Failed to close directory %s\n%s\n", dir_name, strerror(errno));
        return -1;
    }

    return 0;
}

int delete_file(char *full_name, char *name)
{
    if (unlink(full_name) != 0) {
        fprintf(stderr, "Could not delete file %s:\n%s\n", name, strerror(errno));
        return -1;
    } else {
        printf("Deleted %s\n", name);
    }
    return 0;
}

int compare_times(const void *dfile1, const void *dfile2)
{
    time_t t1 = ((struct DFile *) dfile1)->cr_time;
    time_t t2 = ((struct DFile *) dfile2)->cr_time;
    if (t1 < t2) {
        return 1;
    } else if (t1 == t2) {
        return 0;
    } else {
        return -1;
    }
}
