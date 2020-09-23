#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int streq(const char *a, const char *b) {
    return !strcmp(a, b);
}

int ends_with(const char *str, const char *needle, int *len) {
    assert(str);
    assert(needle);
    int nlen = strlen(needle);
    int slen = strlen(str);
    *len = slen;
    if (!nlen || !slen) return 0;
    if (slen < nlen) return 0;
    str = str + slen - nlen;
    while (*str) {
        if (*str++ != *needle++) return 0;
    }
    return 1;
}

int main()
{
    DIR *src;
    struct dirent *entry;

    int ext_len = strlen(".ir");

    const char *dir = "../../IR";

    src = opendir(dir);
    assert(src);
    while ((entry = readdir(src)))
    {
        int namelen;
        if (ends_with(entry->d_name, ".ir", &namelen))
        {
            char buf[512];
            struct stat st;
            printf("- %s\n", entry->d_name);
            sprintf(buf, "./%.*s.out", namelen - ext_len, entry->d_name);
            if (access(buf, F_OK) == -1) {
                printf("\t\033[1;31m No .out \033[0m\n");
                continue;
            }
            sprintf(buf, "../print_nat_loops %s/%s > curr_out", dir, entry->d_name);
            system(buf);
            sprintf(buf, "diff curr_out ./%.*s.out > curr_diff", namelen - ext_len, entry->d_name);
            system(buf);
            system("rm curr_out");
            stat("curr_diff", &st);
            if (st.st_size != 0) {
                printf("MISMATCH in %s\n", entry->d_name);
                break;
            } else {
                printf("\t\033[1;32m SUCCESS \033[0m\n");
                system("rm curr_diff");
            }
        }
    }
    closedir(src);

    return(0);
}
