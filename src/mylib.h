#ifndef MYLIBH
#define MYLIBH

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

typedef struct {
    double tp, iops;
} measres_t;

typedef struct {
    bool is_trial;
    size_t nreads, bsize;
    bool is_o_direct;
    double region;
    size_t nthreads;
    int fd;
    off_t hdd_area;
} paras_t;

extern const char *gHddFile;

double calc_elapsed(struct timeval start_tv, struct timeval end_tv);
void perror_exit(char *msg);
void show_usage(char *program_name);
char *p_bool(bool is_true);
void drop_raid_cache();
void bubble_sort(double arr[], size_t len);
bool is_inner_th(double arr[], size_t len);

#endif
