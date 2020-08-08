#define _GNU_SOURCE

#include "mylib.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BBYTES 1 GiB

FILE *gFP;

void measure_by_addresses(paras_t paras) {
    printf("started measuring\n");

    struct timeval start_tv, end_tv;

    int fd;
    void *blkbuf;
    if(posix_memalign(&blkbuf, 512, BBYTES))
        perror_exit("posix memalign");

    fd = open(HDDFILE, O_RDONLY | O_DIRECT);
    if(fd < 0)
        perror_exit("open error");

    off_t hdd_size = lseek(fd, 0, SEEK_END);
    off_t hdd_ofst = lseek(fd, 0, SEEK_SET);

    double real, tp;
    while(hdd_ofst < hdd_size) {

        gettimeofday(&start_tv, NULL);
        if(read(fd, blkbuf, BBYTES) == -1)
            perror_exit("read error");
        gettimeofday(&end_tv, NULL);

        real = calc_elapsed(start_tv, end_tv);
        tp = ((double)BBYTES / real) * 1e-6; // Seq. Read Throughput (MB/sec)
        fprintf(gFP, "%zu,%f,%zu,%zu,%zu,,%f,,,\n", paras.bsize, paras.region,
                paras.nthreads, hdd_ofst, hdd_size, tp);

        hdd_ofst = lseek(fd, 0, SEEK_CUR);
    }

    close(fd);
    free(blkbuf);
}

int main(int argc, char **argv) {
    if(argc != 2)
        show_usage(argv[0]);
    printf("started measureing disk performance by addresses\n");

    drop_raid_cache();

    if((gFP = fopen(argv[1], "w")) == NULL)
        perror_exit("open error");

    print_csvheaders(gFP, "addresser");

    paras_t paras = {false, NOTNEED, NOTNEED, BBYTES, true,
                     1.,    1,       NOTNEED, NOTNEED};
    measure_by_addresses(paras);

    fclose(gFP);
    return EXIT_SUCCESS;
}
