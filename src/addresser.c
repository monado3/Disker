#define _GNU_SOURCE

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define HDDFILE "/dev/sdb"

#define KiB *1024
#define MiB *1048576
#define GiB *1073741824

#define DRCREADGB 200
#define DRCREADBUFGB 1

#define BBYTES 1 GiB

FILE *gFP;

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

void measure_by_addresses() {
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
        fprintf(gFP, "%zu,%f\n", hdd_ofst, tp);

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

    fprintf(gFP,
            "Disker addresser output\n"
            "disk:%s\n\n"
            "address,direct_tp(MB/sec)\n",
            HDDFILE);

    measure_by_addresses();

    fclose(gFP);
    return EXIT_SUCCESS;
}
