#define _GNU_SOURCE

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

#define HDDFILE "/dev/sdb1"

#define DRCREADGB 200
#define DRCREADBUFGB 1

#define KiB *1024
#define MiB *1048576
#define GiB *1073741824

#define GOODTH 0.1
#define NTRIALS 3 // must be odd

#define DEFBSIZE 4 KiB
#define DEFREGION 1.
#define DEFNTHREADS 1
#define MAXNTREADS 100
#define NOTNEED 0

#define INFOHEADER "disk:%s, nreads: %zu\n\n"
#define CSVHEADER                                                              \
    "bsize,region,nthreads,direct_tp(MB/sec),direct_iops,indirect_tp(MB/"      \
    "sec),indirect_iops\n"

FILE *gFP;
int gNreads;

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

measres_t measure(paras_t paras);

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

char *p_bool(bool is_true) {
    char *t = "true";
    char *f = "false";
    return is_true ? t : f;
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

bool is_good_nreads(paras_t paras) {
    double tps[NTRIALS];
    double iopses[NTRIALS];

    size_t i;
    measres_t res;
    for(i = 0; i < NTRIALS; i++) {
        res = measure(paras);
        tps[i] = res.tp;
        iopses[i] = res.iops;
    }

    bubble_sort(tps);
    bubble_sort(iopses);

    return is_inner_th(tps) && is_inner_th(iopses);
}

size_t search_good_nreads(paras_t paras) {
    size_t len = 7;
    size_t nreadses[] = {500, 1000, 5000, 10000, 50000, 100000, 500000};
    size_t i;
    for(i = 0; i < len; i++) {
        paras.nreads = nreadses[i];
        if(is_good_nreads(paras)) {
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
    bool is_o_direct = paras->is_o_direct;
    int fd = paras->fd;
    off_t hdd_area = paras->hdd_area;

    void *blkbuf;
    if(is_o_direct) {
        if(posix_memalign(&blkbuf, 512, bsize))
            perror_exit("posix memalign");
    } else {
        blkbuf = (char *)malloc(bsize);
    }
    while(gNreads > 0) {
        if(pread(fd, blkbuf, bsize, rand() % hdd_area) == -1)
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

    off_t hdd_area = lseek(fd, 0, SEEK_END) * region;
    lseek(fd, rand() % hdd_area, SEEK_SET);

    size_t i;
    if(nthreads < 2) { // single thread
        void *blkbuf;
        if(is_o_direct) {
            if(posix_memalign(&blkbuf, 512, bsize))
                perror_exit("posix memalign");
        } else {
            blkbuf = (void *)malloc(bsize);
        }
        off_t align_area = hdd_area / 512;
        gettimeofday(&start_tv, NULL);
        for(i = 0; i < nreads; i++) {
            lseek(fd, (rand() % align_area) * 512, SEEK_SET);
            if(read(fd, blkbuf, bsize) == -1)
                perror_exit("read error");
        }
        gettimeofday(&end_tv, NULL);
        free(blkbuf);
    } else { // multi threads
        gNreads = nreads;
        paras.fd = fd;
        paras.hdd_area = hdd_area;
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
        fprintf(gFP, "%zu,%f,%zu,%f,%f", bsize, region, nthreads, tp, iops);
    else
        fprintf(gFP, ",%f,%f\n", tp, iops);

    return res;
}

void measure_by_bsizes(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/01bsize.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    fprintf(gFP,
            "Disker randomer bsizes output\n"
            "disk:%s, nreads: %zu\n\n" CSVHEADER,
            HDDFILE, nreads);

    size_t i, len = 14;
    size_t bsizes[] = {512,   1 KiB, 4 KiB,  16 KiB, 64 KiB,  128 KiB, 512 KiB,
                       1 MiB, 4 MiB, 16 MiB, 64 MiB, 128 MiB, 256 MiB, 512 MiB};
    for(i = 0; i < len; i++) {
        paras_t direct = {false,     nreads,      bsizes[i], true,
                          DEFREGION, DEFNTHREADS, NOTNEED,   NOTNEED};
        paras_t indirect = {false,     nreads,      bsizes[i], false,
                            DEFREGION, DEFNTHREADS, NOTNEED,   NOTNEED};
        measure(direct);
        measure(indirect);
    }
    fclose(gFP);
}

void measure_by_regions(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/02region.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    fprintf(gFP, "Disker randomer regions output\n" INFOHEADER CSVHEADER,
            HDDFILE, nreads);

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t direct = {false,    nreads,      DEFBSIZE, true,
                          i * 0.01, DEFNTHREADS, NOTNEED,  NOTNEED};
        paras_t indirect = {false,    nreads,      DEFBSIZE, false,
                            i * 0.01, DEFNTHREADS, NOTNEED,  NOTNEED};
        measure(direct);
        measure(indirect);
    }
    for(i = 1; i <= len; i++) {
        paras_t direct = {false,   nreads,      DEFBSIZE, true,
                          i * 0.1, DEFNTHREADS, NOTNEED,  NOTNEED};
        paras_t indirect = {false,   nreads,      DEFBSIZE, false,
                            i * 0.1, DEFNTHREADS, NOTNEED,  NOTNEED};
        measure(direct);
        measure(indirect);
    }
    fclose(gFP);
}

void measure_by_threads(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/03threads.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    fprintf(gFP, "Disker randomer threads output\n" INFOHEADER CSVHEADER,
            HDDFILE, nreads);

    size_t i;
    for(i = 1; i <= MAXNTREADS; i++) {
        paras_t direct = {false,     nreads, DEFBSIZE, true,
                          DEFREGION, i,      NOTNEED,  NOTNEED};
        paras_t indirect = {false,     nreads, DEFBSIZE, false,
                            DEFREGION, i,      NOTNEED,  NOTNEED};
        measure(direct);
        measure(indirect);
    }
    fclose(gFP);
}

void measure_by_region_mthreads(size_t nreads, char *logdir) {
    char logpath[100];
    sprintf(logpath, "/%s/04regionmulti.csv", logdir);

    if((gFP = fopen(logpath, "w")) == NULL)
        perror_exit("open error");

    fprintf(
        gFP,
        "Disker randomer regions by mutlithreads output\n" INFOHEADER CSVHEADER,
        HDDFILE, nreads);

    size_t i, len = 10;
    for(i = 1; i < len; i++) {
        paras_t direct = {false,    nreads,     DEFBSIZE, true,
                          i * 0.01, MAXNTREADS, NOTNEED,  NOTNEED};
        paras_t indirect = {false,    nreads,     DEFBSIZE, false,
                            i * 0.01, MAXNTREADS, NOTNEED,  NOTNEED};
        measure(direct);
        measure(indirect);
    }
    for(i = 1; i <= len; i++) {
        paras_t direct = {false,   nreads,     DEFBSIZE, true,
                          i * 0.1, MAXNTREADS, NOTNEED,  NOTNEED};
        paras_t indirect = {false,   nreads,     DEFBSIZE, false,
                            i * 0.1, MAXNTREADS, NOTNEED,  NOTNEED};
        measure(direct);
        measure(indirect);
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
    paras_t trial = {true,      NOTNEED,     DEFBSIZE, true,
                     DEFREGION, DEFNTHREADS, NOTNEED,  NOTNEED};
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
