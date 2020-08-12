#define _GNU_SOURCE

#include "mylib.h"
#include "SFMT.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DRCREADGB 200
#define DRCREADBUFGB 1

#define NTRIALS 3 // must be odd
#define GOODTH 0.1

static sfmt_t sfmt;

// Return a random integer between 0 and max inclusive.
off_t myrand(off_t max) {
    return (off_t)sfmt_genrand_uint64(&sfmt) % (max + 1);
}

void mysrand(uint32_t seed) { sfmt_init_gen_rand(&sfmt, seed); }

double calc_elapsed(struct timeval start_tv, struct timeval end_tv) {
    return (end_tv.tv_sec - start_tv.tv_sec) +
           (end_tv.tv_usec - start_tv.tv_usec) / 1e6;
}

void perror_exit(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void show_usage(char *program_name) {
    fprintf(stderr, "Usage: %s log_file_full_path\n", program_name);
    exit(EXIT_FAILURE);
}

char *p_bool(bool is_true) {
    char *t = "true";
    char *f = "false";
    return is_true ? t : f;
}

void drop_raid_cache() {
    size_t bsize = DRCREADBUFGB * 1e9;

    char *blkbuf = (char *)malloc(bsize);

    int fd = open(HDDFILE, O_RDONLY);
    if(fd < 0)
        perror_exit("open error");

    off_t hdd_size = lseek(fd, 0, SEEK_END);
    int hdd_gb = hdd_size / 1e9;
    printf("%s: size = %d GB\n", HDDFILE, hdd_gb);
    off_t hdd_rear = hdd_size - (DRCREADGB + 1) * 1e9;
    lseek(fd, hdd_rear, SEEK_SET); // set seek to the rear part of HDD

    size_t i;
    size_t nloops = DRCREADGB / DRCREADBUFGB;
    for(i = 0; i < nloops; i++) {
        if(read(fd, blkbuf, bsize) == -1)
            perror_exit("read error");
    }

    close(fd);

    if(system("sync; echo 1 > /proc/sys/vm/drop_caches") == 127)
        perror_exit("drop page cache error");
    else
        printf("droped page cache\n");

    printf("droped RAID cache\n");
}

void bubble_sort(double arr[], size_t len) {
    size_t i, j;
    double tmp;
    for(i = 0; i < len - 1; i++) {
        for(j = 0; j < i; j--) {
            if(arr[j - 1] > arr[j]) {
                tmp = arr[j];
                arr[j] = arr[j - 1];
                arr[j - 1] = tmp;
            }
        }
    }
}

bool is_inner_th(double arr[], size_t len) {
    double base = arr[(len - 1) / 2];
    if((1 - GOODTH) * base < arr[0]) {
        if(arr[0] < (1 + GOODTH) * base) {
            return true;
        }
    }
    return false;
}

bool is_good_para(measres_t (*func)(paras_t), paras_t paras) {
    double tps[NTRIALS];
    double iopses[NTRIALS];

    size_t i;
    measres_t res;
    for(i = 0; i < NTRIALS; i++) {
        res = (*func)(paras);
        tps[i] = res.tp;
        iopses[i] = res.iops;
    }

    bubble_sort(tps, NTRIALS);
    bubble_sort(iopses, NTRIALS);

    return is_inner_th(tps, NTRIALS) && is_inner_th(iopses, NTRIALS);
}

void print_csvheaders(FILE *fp, char *progname) {
    fprintf(fp,
            "Disker %s output\n"
            "disk:%s\n\n"

            "bsize,region,nthreads,address,readbytes,nreads,direct_tp(MB/"
            "sec),direct_iops,indirect_tp(MB/sec),indirect_iops\n",
            progname, HDDFILE);
}
