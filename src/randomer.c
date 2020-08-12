#define _GNU_SOURCE

#include "mylib.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFBSIZE 512
#define DEFREGION 1.
#define DEFNTHREADS 1
#define MAXNTREADS 100

FILE *gFP;
int gNreads;

measres_t measure(paras_t paras);

size_t search_good_nreads(paras_t paras) {
    size_t len = 7;
    size_t nreadses[] = {500, 1000, 5000, 10000, 50000, 100000, 500000};
    size_t i;
    for(i = 0; i < len; i++) {
        paras.nreads = nreadses[i];
        if(is_good_para(measure, paras)) {
            printf("found good nreads: %zu\n", nreadses[i]);
            return nreadses[i];
        }
    }
    perror_exit("found no good nreads");
    return 1;
}

void *p_measure(void *p) {
    paras_t *paras = (paras_t *)p;
    size_t bsize = paras->bsize;
    int fd = paras->fd;
    off_t align_space = paras->align_space;

    void *blkbuf;
    if(posix_memalign(&blkbuf, 512, bsize))
        perror_exit("posix memalign");
    while(gNreads > 0) {
        if(pread(fd, blkbuf, bsize, (rand() % align_space) * 512) == -1)
            perror_exit("pread error");
        gNreads--;
    }
    return (void *)NULL;
}

measres_t measure(paras_t paras) {
    bool is_trial = paras.is_trial;
    size_t nreads = paras.nreads;
    size_t bsize = paras.bsize;
    bool is_o_direct = paras.is_o_direct;
    double region = paras.region;
    size_t nthreads = paras.nthreads;

    printf("started measure(): trial=%s, nreads=%zu, bsize=%zu, direct=%s, "
           "region=%f, nthreads=%zu\n",
           p_bool(is_trial), nreads, bsize, p_bool(is_o_direct), region,
           nthreads);

    struct timeval start_tv, end_tv;

    int fd;
    if(is_o_direct)
        fd = open(HDDFILE, O_RDONLY | O_DIRECT);
    else
        fd = open(HDDFILE, O_RDONLY);
    if(fd < 0)
        perror_exit("open error");

    off_t align_space = (lseek(fd, 0, SEEK_END) * region) / 512;
    lseek(fd, (rand() % align_space) * 512, SEEK_SET);

    size_t i;
    if(nthreads < 2) { // single thread
        void *blkbuf;
        if(posix_memalign(&blkbuf, 512, bsize))
            perror_exit("posix memalign");
        gettimeofday(&start_tv, NULL);
        for(i = 0; i < nreads; i++) {
            lseek(fd, (rand() % align_space) * 512, SEEK_SET);
            if(read(fd, blkbuf, bsize) == -1)
                perror_exit("read error");
        }
        gettimeofday(&end_tv, NULL);
        free(blkbuf);
    } else { // multi threads
        gNreads = nreads;
        paras.fd = fd;
        paras.align_space = align_space;
        pthread_t pthreads[nthreads];
        for(i = 0; i < nthreads; i++) {
            pthread_t p;
            pthreads[i] = p;
        }
        gettimeofday(&start_tv, NULL);
        for(i = 0; i < nthreads; i++) {
            pthread_create(&pthreads[i], NULL, p_measure, &paras);
        }
        for(i = 0; i < nthreads; i++) {
            pthread_join(pthreads[i], NULL);
        }
        gettimeofday(&end_tv, NULL);
    }
    close(fd);

    double real = calc_elapsed(start_tv, end_tv);
    double tp =
        ((double)bsize * nreads / real) * 1e-6; // Seq. Read Throughput (MB/sec)
    double iops = nreads / real;
    measres_t res = {tp, iops};

    if(is_trial)
        return res;

    if(is_o_direct)
        fprintf(gFP, "%zu,%f,%zu,,,%zu,%f,%f", bsize, region, nthreads, nreads,
                tp, iops);
    else
        fprintf(gFP, ",%f,%f\n", tp, iops);

    return res;
}

void measure_by_bsizes(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/01bsize.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer bsizes");

    size_t i, len = 14;
    size_t bsizes[] = {512,   1 KiB, 4 KiB,  16 KiB, 64 KiB,  128 KiB, 512 KiB,
                       1 MiB, 4 MiB, 16 MiB, 64 MiB, 128 MiB, 256 MiB, 512 MiB};
    for(i = 0; i < len; i++) {
        paras_t paras = {false,     NOTNEED,     nreads,  bsizes[i], true,
                         DEFREGION, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

void measure_by_regions(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/02region.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer regions");

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t paras = {false,    NOTNEED,     nreads,  DEFBSIZE, true,
                         i * 0.01, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    for(i = 1; i <= len; i++) {
        paras_t paras = {false,   NOTNEED,     nreads,  DEFBSIZE, true,
                         i * 0.1, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;

        measure(paras);
    }
    fclose(gFP);
}

void measure_by_threads(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/03threads.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer threads");

    size_t i;
    for(i = 1; i <= MAXNTREADS; i++) {
        paras_t paras = {false,     NOTNEED, nreads,  DEFBSIZE, true,
                         DEFREGION, i,       NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

void measure_by_region_mthreads(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/04regionmulti.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer regions by mutlithreads");

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t paras = {false,    NOTNEED,    nreads,  DEFBSIZE, true,
                         i * 0.01, MAXNTREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    for(i = 1; i <= len; i++) {
        paras_t paras = {false,   NOTNEED,    nreads,  DEFBSIZE, true,
                         i * 0.1, MAXNTREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

int main(int argc, char **argv) {
    if(argc != 2)
        show_usage(argv[0]);
    char *logdir = argv[1];
    printf("started measureing random disk performance\n");

    drop_raid_cache();

    size_t nreads;
    paras_t trial = {true,      NOTNEED,     NOTNEED, DEFBSIZE, true,
                     DEFREGION, DEFNTHREADS, NOTNEED, NOTNEED};
    nreads = search_good_nreads(trial);
    measure_by_bsizes(nreads, logdir);

    trial.region = 0.1;
    nreads = search_good_nreads(trial);
    measure_by_regions(nreads, logdir);
    trial.region = DEFREGION;

    trial.nthreads = MAXNTREADS;
    nreads = search_good_nreads(trial);
    measure_by_threads(nreads, logdir);

    measure_by_region_mthreads(nreads, logdir);

    return EXIT_SUCCESS;
}
