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

#define BBYTES 1 GiB

FILE *gFP;

void measure_by_addresses(paras_t paras) {
    RW rw = paras.rw;
    size_t bsize = paras.bsize;
    size_t nthreads = paras.nthreads;

    printf("started measuring\n");

    struct timeval start_tv, end_tv;

    int fd;
    void *blkbuf;
    if(posix_memalign(&blkbuf, 512, BBYTES))
        perror_exit("posix memalign");
    if(rw == WRITE)
        memset(blkbuf, rand(), bsize);

    fd = open(HDDFILE, rw | O_DIRECT);
    if(fd < 0)
        perror_exit("open error");

    off_t hdd_size = lseek(fd, 0, SEEK_END);
    off_t hdd_ofst = lseek(fd, 0, SEEK_SET);

    double real, tp;
    while(hdd_ofst < hdd_size) {

        switch(rw) {
        case READ:
            gettimeofday(&start_tv, NULL);
            if(read(fd, blkbuf, BBYTES) == -1)
                perror_exit("read error");
            gettimeofday(&end_tv, NULL);
            break;
        case WRITE:
            gettimeofday(&start_tv, NULL);
            if(write(fd, blkbuf, BBYTES) == -1)
                perror_exit("write error");
            gettimeofday(&end_tv, NULL);
            break;
        }

        real = calc_elapsed(start_tv, end_tv);
        tp = ((double)BBYTES / real) * 1e-6; // Seq. Throughput (MB/sec)
        fprintf(gFP, "%s,%zu,,%zu,%zu,%zu,,%f,,,\n", p_rw(rw), bsize, nthreads,
                hdd_ofst, hdd_size, tp);

        hdd_ofst = lseek(fd, 0, SEEK_CUR);
    }

    close(fd);
    free(blkbuf);
}

int main(int argc, char **argv) {
    RW rw = argparse(argc, argv);
    printf("started measureing disk performance by addresses\n");

    drop_raid_cache();

    if((gFP = fopen(argv[2], "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "addresser");

    paras_t paras = {rw,   false, NOTNEED, NOTNEED, BBYTES,
                     true, 1.,    1,       NOTNEED, NOTNEED};
    measure_by_addresses(paras);

    fclose(gFP);
    return EXIT_SUCCESS;
}
