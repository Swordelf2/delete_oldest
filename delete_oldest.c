#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>

#define _BSD_SOURCE

const char HELP_STR[] = "delete_oldest deletes the oldest file in given directory with given prefix and suffix If DAYS is given then deletes all such files older than DAYS days.\n";
const char USAGE_STR[] = "Usage: delete_oldest DIR PREFIX SUFFIX [DAYS]\n";


enum Args
{
    ARG_CNT = 3 + 1,
    ARG_DIR = 1,
    ARG_PREFIX,
    ARG_SUFFIX,
    ARG_DAYS
};

enum
{
    MAX_ARG_SIZE = 1024,
    MAX_ERROR_STR_SIZE = 1024,
    SECS_IN_DAY = 24 * 60 * 60
};

int delete_oldest(char *dir_name, char *prefix, char *suffix, int days);
inline int delete_file(char *full_name, char *name);

int main(int argc, char *argv[])
{
    if (argc >= 2 &&
            (strncmp(argv[1], "-h", MAX_ARG_SIZE) == 0 || strncmp(argv[1], "--help", MAX_ARG_SIZE) == 0)) {
        fputs(HELP_STR, stdout);
        fputs(USAGE_STR, stdout);
        // provide help info
    } else if (argc >= ARG_CNT) {
        int days = -1;
        if (argc >= ARG_CNT + 1) {
            days = strtol(argv[ARG_DAYS], NULL, 0);
        }
        return delete_oldest(argv[ARG_DIR], argv[ARG_PREFIX], argv[ARG_SUFFIX], days);
    } else {
        fputs(USAGE_STR, stdout);
        return 1;
    }
}

int delete_oldest(char *dir_name, char *prefix, char *suffix, int days)
{
    size_t pref_len = strlen(prefix);
    size_t suf_len = strlen(suffix);
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        fputs("Could not open director\n", stderr);
        return -1;
    }

    struct dirent *dir_ent;
    char min_name[PATH_MAX];
    int min_found = 0;
    time_t min_time = 0;
    while ((dir_ent = readdir(dir))) {
        char file_name[PATH_MAX];
        char *name = dir_ent->d_name;
        size_t name_len = strlen(name);
        // check prefix and suffix
        if (strncmp(name, prefix, pref_len) == 0 &&
            (name_len >= suf_len && strncmp(name + name_len - suf_len, suffix, suf_len) ==0)) {
            // now checkwhether it's a regualr file
            snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, name);
            struct stat file_stat;
            if (lstat(file_name, &file_stat) != 0) {
                char error_str[MAX_ERROR_STR_SIZE];
                snprintf(error_str, sizeof(error_str), "Could not stat file %s\n", name);
                fputs(error_str, stderr);
                return -1;
            }

            if (S_ISREG(file_stat.st_mode)) {
                // check the date if needed
                if (days >= 0 && difftime(time(NULL), file_stat.st_mtime) > (double) days * SECS_IN_DAY) {
                    delete_file(file_name, name);
                } else if (days == -1) {
                    if (!min_found || file_stat.st_mtime < min_time) {
                        min_found = 1;
                        strcpy(min_name, file_name);
                    }
                }
            }
        }
    }
    if (closedir(dir) != 0) {
        fputs("Failed to close directory\n", stderr);
        return -1;
    }

    if (min_found) {
        delete_file(min_name, min_name);
    }

    return 0;
}

int delete_file(char *full_name, char *name)
{
    char error_str[MAX_ERROR_STR_SIZE];
    if (unlink(full_name) != 0) {
        snprintf(error_str, sizeof(error_str), "Could not delete file %s\n", name);
        fputs(error_str, stderr);
        return -1;
    } else {
        snprintf(error_str, sizeof(error_str), "Deleted %s\n", name);
        fputs(error_str, stdout);
    }
    return 0;
}
