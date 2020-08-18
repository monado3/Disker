#ifdef __linux__
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef MYLIBH
#define MYLIBH

#include "SFMT.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

// #define HDDFILE "/dev/sdb1" // for kugenuma26
#define HDDFILE "/dev/sdb" // for kugenuma29

#define KiB *1024
#define MiB *1048576
#define GiB *1073741824

#define NOTNEED 0

typedef enum {
    READ, // = O_RDONLY = 0
    WRITE,  // = O_WRONLY = 1
} RW;

typedef struct {
    double tp, iops;
} measres_t;

typedef struct {
    RW rw;
    bool is_trial;
    size_t rwbytes, nios, bsize;
    bool is_o_direct;
    double region;
    size_t nthreads;
    int fd;
    off_t align_space;
} paras_t;

off_t myrand(off_t max, sfmt_t *sfmt);
RW argparse(int argc, char **argv);
double calc_elapsed(struct timeval start_tv, struct timeval end_tv);
void perror_exit(char *msg);
void show_usage(char *program_name);
char *p_bool(bool is_true);
char *p_rw(RW rw);
void drop_raid_cache();
void bubble_sort(double arr[], size_t len);
bool is_inner_th(double arr[], size_t len);
bool is_good_para(measres_t (*func)(paras_t), paras_t paras);
void print_csvheaders(FILE *fp, char *progname);

#endif
