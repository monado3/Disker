#ifndef MYLIBH
#define MYLIBH

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#define HDDFILE "/dev/sdb1"

#define KiB *1024
#define MiB *1048576
#define GiB *1073741824

#define NOTNEED 0

typedef struct {
    double tp, iops;
} measres_t;

typedef struct {
    bool is_trial;
    size_t readbytes, nreads, bsize;
    bool is_o_direct;
    double region;
    size_t nthreads;
    int fd;
    off_t hdd_area;
} paras_t;

double calc_elapsed(struct timeval start_tv, struct timeval end_tv);
void perror_exit(char *msg);
void show_usage(char *program_name);
char *p_bool(bool is_true);
void drop_raid_cache();
void bubble_sort(double arr[], size_t len);
bool is_inner_th(double arr[], size_t len);
bool is_good_para(measres_t (*func)(paras_t), paras_t paras);
void print_csvheaders(FILE *fp, char *progname);

#endif
