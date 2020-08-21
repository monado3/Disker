#include "mylib.h"
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
static atomic_long gNios;

measres_t measure(paras_t paras);

size_t search_good_nios(paras_t paras) {
    size_t len = 7;
    size_t nioses[] = {500, 1000, 5000, 10000, 50000, 100000, 500000};
    size_t i;
    for(i = 0; i < len; i++) {
        paras.nios = nioses[i];
        if(is_good_para(measure, paras)) {
            printf("found good nios: %zu\n", nioses[i]);
            return nioses[i];
        }
    }
    perror_exit("found no good nios");
    return 1;
}

void *p_measure(void *p) {
    paras_t *paras = (paras_t *)p;
    RW rw = paras->rw;
    size_t bsize = paras->bsize;
    int fd = paras->fd;
    off_t align_space = paras->align_space;

    void *blkbuf;
    if(posix_memalign(&blkbuf, 512, bsize))
        perror_exit("posix memalign");
    if(rw == WRITE)
        memset(blkbuf, rand(), bsize);

    sfmt_t sfmt;
    sfmt_init_gen_rand(&sfmt, rand());
    switch(rw) {
    case READ:
        while(gNios > 0) {
            if(pread(fd, blkbuf, bsize, myrand(align_space, &sfmt) * 512) == -1)
                perror_exit("pread error");
            gNios--;
        }
        break;
    case WRITE:
        while(gNios > 0) {
            if(pwrite(fd, blkbuf, bsize, myrand(align_space, &sfmt) * 512) ==
               -1)
                perror_exit("pwrite error");
            gNios--;
        }
        break;
    }
    return (void *)NULL;
}

measres_t measure(paras_t paras) {
    RW rw = paras.rw;
    bool is_trial = paras.is_trial;
    size_t nios = paras.nios;
    size_t bsize = paras.bsize;
    bool is_o_direct = paras.is_o_direct;
    double region = paras.region;
    size_t nthreads = paras.nthreads;

    printf(
        "started measure(): rw=%s, trial=%s, nios=%zu, bsize=%zu, direct=%s, "
        "region=%f, nthreads=%zu\n",
        p_rw(rw), p_bool(is_trial), nios, bsize, p_bool(is_o_direct), region,
        nthreads);

    struct timeval start_tv, end_tv;

    int fd;
    if(is_o_direct)
        fd = open(HDDFILE, rw | O_DIRECT);
    else
        fd = open(HDDFILE, rw);
    if(fd < 0)
        perror_exit("open error");

    off_t align_space = (lseek(fd, 0, SEEK_END) * region) / 512;
    assert(align_space > 0);

    size_t i;
    if(nthreads < 2) { // single thread
        void *blkbuf;
        if(posix_memalign(&blkbuf, 512, bsize))
            perror_exit("posix memalign");
        if(rw == WRITE)
            memset(blkbuf, rand(), bsize);
        sfmt_t sfmt;
        sfmt_init_gen_rand(&sfmt, rand());
        switch(rw) {
        case READ:
            gettimeofday(&start_tv, NULL);
            for(i = 0; i < nios; i++) {
                if(lseek(fd, myrand(align_space, &sfmt) * 512, SEEK_SET) == -1)
                    perror_exit("lseek error");
                if(read(fd, blkbuf, bsize) == -1)
                    perror_exit("read error");
            }
            gettimeofday(&end_tv, NULL);
            break;
        case WRITE:
            gettimeofday(&start_tv, NULL);
            for(i = 0; i < nios; i++) {
                if(lseek(fd, myrand(align_space, &sfmt) * 512, SEEK_SET) == -1)
                    perror_exit("lseek error");
                if(write(fd, blkbuf, bsize) == -1)
                    perror_exit("write error");
            }
            gettimeofday(&end_tv, NULL);
            break;
        }
        free(blkbuf);
    } else { // multi threads
        gNios = nios;
        paras.fd = fd;
        paras.align_space = align_space;
        pthread_t pthreads[nthreads];
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
    double tp = ((double)bsize * nios / real) * 1e-6; // Throughput (MB/sec)
    double iops = nios / real;
    measres_t res = {tp, iops};

    if(is_trial)
        return res;

    if(is_o_direct)
        fprintf(gFP, "%s,%zu,%f,%zu,,,%zu,%f,%f", p_rw(rw), bsize, region,
                nthreads, nios, tp, iops);
    else
        fprintf(gFP, ",%f,%f\n", tp, iops);

    return res;
}

void measure_by_bsizes(RW rw, size_t nios, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/01bsize.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer bsizes");

    size_t i, len = 22;
    size_t bsizes[] = {512,     1 KiB,   4 KiB,   16 KiB, 64 KiB, 128 KiB,
                       512 KiB, 1 MiB,   2 MiB,   3 MiB,  4 MiB,  6 MiB,
                       8 MiB,   10 MiB,  12 MiB,  14 MiB, 16 MiB, 32 MiB,
                       64 MiB,  128 MiB, 256 MiB, 512 MiB};
    for(i = 0; i < len; i++) {
        paras_t paras = {rw,   false,     NOTNEED,     nios,    bsizes[i],
                         true, DEFREGION, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

void measure_by_regions(RW rw, size_t nios, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/02region.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer regions");

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t paras = {rw,   false,    NOTNEED,     nios,    DEFBSIZE,
                         true, i * 0.01, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    for(i = 1; i <= len; i++) {
        paras_t paras = {rw,   false,   NOTNEED,     nios,    DEFBSIZE,
                         true, i * 0.1, DEFNTHREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;

        measure(paras);
    }
    fclose(gFP);
}

void measure_by_threads(RW rw, size_t nios, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/03threads.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer threads");

    size_t i;
    for(i = 1; i <= MAXNTREADS; i++) {
        paras_t paras = {rw,   false,     NOTNEED, nios,    DEFBSIZE,
                         true, DEFREGION, i,       NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

void measure_by_region_mthreads(RW rw, size_t nios, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/04regionmulti.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "randomer regions by mutlithreads");

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t paras = {rw,   false,    NOTNEED,    nios,    DEFBSIZE,
                         true, i * 0.01, MAXNTREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    for(i = 1; i <= len; i++) {
        paras_t paras = {rw,   false,   NOTNEED,    nios,    DEFBSIZE,
                         true, i * 0.1, MAXNTREADS, NOTNEED, NOTNEED};
        measure(paras);
        paras.is_o_direct = false;
        measure(paras);
    }
    fclose(gFP);
}

int main(int argc, char **argv) {
    RW rw = argparse(argc, argv);
    char *logdir = argv[2];
    printf("started measureing random disk performance\n");

    drop_raid_cache();

    size_t nios;
    paras_t trial = {rw,   true,      NOTNEED,     NOTNEED, DEFBSIZE,
                     true, DEFREGION, DEFNTHREADS, NOTNEED, NOTNEED};
    nios = search_good_nios(trial);
    measure_by_bsizes(rw, nios, logdir);

    trial.region = 0.01;
    nios = search_good_nios(trial);
    measure_by_regions(rw, nios, logdir);
    trial.region = DEFREGION;

    trial.nthreads = MAXNTREADS;
    nios = search_good_nios(trial);
    measure_by_threads(rw, nios, logdir);

    measure_by_region_mthreads(rw, nios, logdir);

    return EXIT_SUCCESS;
}
