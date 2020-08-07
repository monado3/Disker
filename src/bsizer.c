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

#define HDDFILE "/dev/sdb1"

#define DRCREADGB 200
#define DRCREADBUFGB 1

#define KiB *1024
#define MiB *1048576
#define GiB *1073741824

#define GOODTH 0.1
#define NTRIALS 3 // must be odd

FILE *gFP;

typedef struct {
    double tp, iops;
} measres_t;

measres_t measure_by_bsize(bool is_o_direct, bool is_trial, size_t bsize,
                           size_t readbytes);

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

void bubble_sort(double arr[]) {
    size_t i, j;
    double tmp;
    for(i = 0; i < NTRIALS - 1; i++) {
        for(j = 0; j < i; j--) {
            if(arr[j - 1] > arr[j]) {
                tmp = arr[j];
                arr[j] = arr[j - 1];
                arr[j - 1] = tmp;
            }
        }
    }
}

bool is_inner_th(double arr[]) {
    double base = arr[(NTRIALS - 1) / 2];
    if((1 - GOODTH) * base < arr[0]) {
        if(arr[0] < (1 + GOODTH) * base) {
            return true;
        }
    }
    return false;
}

bool is_good_readbyte(size_t readbytes) {
    double tps[NTRIALS];
    double iopses[NTRIALS];

    size_t i;
    measres_t res;
    for(i = 0; i < NTRIALS; i++) {
        res = measure_by_bsize(true, true, 64 KiB, readbytes);
        tps[i] = res.tp;
        iopses[i] = res.iops;
    }

    bubble_sort(tps);
    bubble_sort(iopses);

    return is_inner_th(tps) && is_inner_th(iopses);
}

size_t search_good_readbytes() {
    size_t len = 3;
    size_t opts[] = {100 MiB, 500 MiB, 1 GiB};
    size_t i;
    for(i = 0; i < len; i++) {
        if(is_good_readbyte(opts[i])) {
            printf("found good readbytes: %zu\n", opts[i]);
            return opts[i];
        }
    }
    perror_exit("found no good readbytes");
}

measres_t measure_by_bsize(bool is_o_direct, bool is_trial, size_t bsize,
                           size_t readbytes) {
    printf("started measuring disk performance by %zu\n", readbytes);
    static off_t sHddOfst = 0;

    struct timeval start_tv, end_tv;

    int fd;
    char *blkbuf;
    if(is_o_direct) {
        if(posix_memalign(&blkbuf, 512, bsize))
            perror_exit("posix memalign");
        fd = open(HDDFILE, O_RDONLY | O_DIRECT);
    } else {
        blkbuf = (char *)malloc(bsize);
        fd = open(HDDFILE, O_RDONLY);
    }

    if(fd < 0)
        perror_exit("open error");

    lseek(fd, sHddOfst, SEEK_SET);

    size_t nreads;
    size_t nloops = readbytes / bsize;
    gettimeofday(&start_tv, NULL);
    for(nreads = 0; nreads < nloops; nreads++) {
        if(read(fd, blkbuf, bsize) == -1)
            perror_exit("read error");
    }
    gettimeofday(&end_tv, NULL);
    sHddOfst = lseek(fd, 0, SEEK_CUR);

    close(fd);
    free(blkbuf);

    double real = calc_elapsed(start_tv, end_tv);
    double tp =
        (double)bsize * nloops * 1e3 / real; // Seq. Read Throughput (MB/sec)
    double iops = nreads / real;
    measres_t res = {tp, iops};

    if(is_trial)
        return res;

    if(is_o_direct)
        fprintf(gFP, "%zu,%f,%f", bsize, tp, iops);
    else
        fprintf(gFP, "%f,%f\n", tp, iops);

    return res;
}

void measure_by_bsizes(size_t readbytes) {
    size_t i, len = 10;
    size_t bsizes[] = {512,   1 KiB, 4 KiB,  64 KiB,  512 KiB,
                       1 MiB, 4 MiB, 64 MiB, 512 MiB, 1 GiB};
    for(i = 0; i < len; i++) {
        measure_by_bsize(true, false, bsizes[i], readbytes);
        measure_by_bsize(false, false, bsizes[i], readbytes);
    }
}

int main(int argc, char **argv) {
    if(argc != 2)
        show_usage(argv[0]);
    printf("started measureing disk performance by bsizes\n");

    drop_raid_cache();
    size_t readbytes = search_good_readbytes();

    if((gFP = fopen(argv[1], "w")) == NULL)
        perror_exit("open error");

    fprintf(gFP,
            "Disker bsizer output\n"
            "disk:%s, readbytes: %zu\n\n"
            "bsize,direct_tp,direct_iops,indirect_tp,indirect_iops\n",
            HDDFILE, readbytes);

    measure_by_bsizes(readbytes);

    fclose(gFP);
    return EXIT_SUCCESS;
}
