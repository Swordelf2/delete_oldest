#define _BSD_SOURCE
#define _FILE_OFFSET_BITS 64

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>
#include <errno.h>

enum Args
{
    ARG_COUNT = 1 + 1,
    ARG_DIR = 1,
};

enum
{
    MAX_FILE_CNT = 512,
    MAX_ARG_SIZE = 1024,
    MAX_ERROR_STR_SIZE = 1024,
    ID_LEN = 5,

    MONTH_NUM = 20,
    MONTH_CNT = 12,
    WEEK_NUM = 10,
    WEEK_CNT = 1,
    DAY_NUM = 1,
    DAY_CNT = 5
};

struct FileID
{
    char *name;
    unsigned id;
};

const char pref_str[] = "backup";
const char suf_str[] = ".tar.gz";

const size_t pref_len = strlen(pref_str);
const size_t suf_len = strlen(suf_str);

int delete_files(DIR *dir, const char *dir_name);
int print_next_file(DIR *dir);
int compare_fileid(const void *f1, const void *f2);
void perform_cleanup(struct FileID *files, int *delete, size_t files_cnt, unsigned num, unsigned cnt);
long long check_name(const char *name);

int main(int argc, char *argv[])
{
    if (argc < ARG_COUNT) {
        return 1;
    }

    char *dir_name = argv[ARG_DIR];
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        fprintf(stderr, "Could not open directory %s:\n%s\n", dir_name, strerror(errno));
        return -1;
    }

    if (argc == ARG_COUNT) {
        delete_files(dir, dir_name);
    } else {
        print_next_file(dir);
    }

    if (closedir(dir) != 0) {
        fprintf(stderr, "Failed to close directory %s\n%s\n", dir_name, strerror(errno));
        return -1;
    }
    return 0;
}

long long check_name(const char *name)
{
    size_t name_len = strlen(name);
    if (name_len == pref_len + ID_LEN + suf_len &&
            strncmp(name, pref_str, pref_len) == 0 &&
            strncmp(name + name_len - suf_len, suf_str, suf_len) == 0) {
        unsigned id;
        sscanf(name + pref_len, "%u", &id);
        return id;
    } else {
        return -1;
    }
}

int delete_files(DIR *dir, const char *dir_name)
{
    struct FileID files[MAX_FILE_CNT];
    size_t files_cnt = 0;


    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir))) {
        char *name = dir_ent->d_name;
        long long id = check_name(name);
        if (id != -1) {
            files[files_cnt].name = strdup(name);
            files[files_cnt].id = id;
            ++files_cnt;
        }
    }

    qsort(files, files_cnt, sizeof(*files), compare_fileid);
    int *delete = malloc(sizeof(*delete) * files_cnt);
    for (unsigned i = 0; i < files_cnt; ++i) {
        delete[i] = 1;
    }

    perform_cleanup(files, delete, files_cnt, DAY_NUM, DAY_CNT);
    perform_cleanup(files, delete, files_cnt, WEEK_NUM, WEEK_CNT);
    perform_cleanup(files, delete, files_cnt, MONTH_NUM, MONTH_CNT);

    for (unsigned i = 0; i < files_cnt; ++i) {
        if (delete[i]) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", dir_name, files[i].name);
            if (unlink(path) == 00) {
                printf("File %s deleted\n", files[i].name);
            } else {
                fprintf(stderr, "Could not delete file %s:\n%s",
                        files[i].name, strerror(errno));
            }
        }
    }
    return 0;
}

int print_next_file(DIR *dir)
{
    unsigned max_id = 0;
    struct dirent *dir_ent;
    while ((dir_ent = readdir(dir))) {
        char *name = dir_ent->d_name;
        long long id = check_name(name);
        if (id != -1) {
            if (id > max_id) {
                max_id = id;
            }
        }
    }

    printf("%s%05u%s", pref_str, max_id, suf_str);
    return 0;
}

void perform_cleanup(struct FileID *files, int *delete, size_t files_cnt, unsigned num, unsigned cnt)
{
    unsigned ind = 0;
    for (unsigned i = 0; i < cnt; ++i) {
        while (ind < files_cnt &&
                !(delete[ind] == 1 && files[ind].id % num == 0)) {
            ++ind;
        }
        if (ind < files_cnt) {
            delete[ind] = 0;
        } else {
            break;
        }
    }
}



int compare_fileid(const void *f1, const void *f2)
{
    unsigned id1 = ((struct FileID *) f1)->id;
    unsigned id2 = ((struct FileID *) f2)->id;
    if (id1 < id2) {
        return 1;
    } else if (id1 == id2) {
        return 0;
    } else {
        return -1;
    }
}
