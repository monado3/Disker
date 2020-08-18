#include "mylib.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BSIZE 64 KiB

FILE *gFP;

measres_t measure_by_bsize(paras_t paras);

size_t search_good_rwbytes(RW rw) {
    size_t len = 2;
    size_t opts[] = {512 MiB, 1 GiB};
    paras_t paras = {rw,   true, NOTNEED, NOTNEED, BSIZE,
                     true, 1.,   1,       NOTNEED, NOTNEED};
    size_t i;
    for(i = 0; i < len; i++) {
        paras.rwbytes = opts[i];
        if(is_good_para(measure_by_bsize, paras)) {
            printf("found good rwbytes: %zu\n", opts[i]);
            return opts[i];
        }
    }
    perror_exit("found no good rwbytes");
    return 1;
}

measres_t measure_by_bsize(paras_t paras) {
    RW rw = paras.rw;
    bool is_trial = paras.is_trial;
    size_t rwbytes = paras.rwbytes;
    size_t bsize = paras.bsize;
    bool is_o_direct = paras.is_o_direct;
    size_t nthreads = paras.nthreads;

    printf("started measuring disk performance by %zu\n", bsize);
    static off_t sHddOfst = 0;

    struct timeval start_tv, end_tv;

    void *blkbuf;
    if(posix_memalign(&blkbuf, 512, bsize))
        perror_exit("posix memalign");
    if(rw == WRITE)
        memset(blkbuf, rand(), bsize);

    int fd;
    if(is_o_direct)
        fd = open(HDDFILE, rw | O_DIRECT);
    else
        fd = open(HDDFILE, rw);
    if(fd < 0)
        perror_exit("open error");

    lseek(fd, sHddOfst, SEEK_SET);

    size_t nios;
    size_t nloops = rwbytes / bsize;

    switch(rw) {
    case READ:
        gettimeofday(&start_tv, NULL);
        for(nios = 0; nios < nloops; nios++) {
            if(read(fd, blkbuf, bsize) == -1)
                perror_exit("read error");
        }
        gettimeofday(&end_tv, NULL);
        break;
    case WRITE:
        gettimeofday(&start_tv, NULL);
        for(nios = 0; nios < nloops; nios++) {
            if(write(fd, blkbuf, bsize) == -1)
                perror_exit("write error");
        }
        gettimeofday(&end_tv, NULL);
        break;
    }
    sHddOfst = lseek(fd, 0, SEEK_CUR);

    close(fd);
    free(blkbuf);

    double real = calc_elapsed(start_tv, end_tv);
    double tp =
        ((double)bsize * nloops / real) * 1e-6; // Seq. Throughput (MB/sec)
    double iops = nios / real;
    measres_t res = {tp, iops};

    if(is_trial)
        return res;

    if(is_o_direct)
        fprintf(gFP, "%s,%zu,,%zu,,%zu,%zu,%f,%f", p_rw(rw), bsize, nthreads,
                rwbytes, nios, tp, iops);
    else
        fprintf(gFP, ",%f,%f\n", tp, iops);

    return res;
}

void measure_by_bsizes(paras_t paras) {
    size_t i, len = 14;
    size_t bsizes[] = {512,   1 KiB, 4 KiB,  16 KiB, 64 KiB,  128 KiB, 512 KiB,
                       1 MiB, 4 MiB, 16 MiB, 64 MiB, 128 MiB, 256 MiB, 512 MiB};
    for(i = 0; i < len; i++) {
        paras.bsize = bsizes[i];
        paras.is_o_direct = true;
        measure_by_bsize(paras);
        paras.is_o_direct = false;
        measure_by_bsize(paras);
    }
}

int main(int argc, char **argv) {
    RW rw = argparse(argc, argv);
    printf("started measureing disk performance by bsizes\n");

    drop_raid_cache();
    size_t rwbytes = search_good_rwbytes(rw);

    if((gFP = fopen(argv[2], "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "bsizer");

    paras_t paras = {rw,      false, rwbytes, NOTNEED, NOTNEED,
                     NOTNEED, 1.,    1,       NOTNEED, NOTNEED};
    measure_by_bsizes(paras);

    fclose(gFP);
    return EXIT_SUCCESS;
}
