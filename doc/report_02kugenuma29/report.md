# Experiment Report 02
---

## Env
```
[Server Info]
kugenuma29

[Kernel Info]
4.18.0-193.14.2.el8_2.x86_64

[CPU Info]
Model name:          Intel(R) Xeon(R) Gold 6132 CPU @ 2.60GHz
NUMA node(s):        2
Socket(s):           2
Core(s) per socket:  14
Thread(s) per core:  2
CPU(s):              56

[Memory Info]
93.0435 GB

[Storage Info]
/dev/sdb
           *-disk:1
                description: SCSI Disk
                product: AL15SEB120NY
                vendor: TOSHIBA
                physical id: 2.1.0
                bus info: scsi@0:2.1.0
                logical name: /dev/sdb
                version: EF06
                serial: 58U0A1UUFQVF
                size: 1117GiB (1200GB)
                capacity: 1835GiB (1971GB)
                capabilities: 10000rpm
                configuration: ansiversion=6 logicalsectorsize=512 sectorsize=512
```

## Results
### Seq.
#### by address
![](s_address.png)

#### by block size
![](s_bsize.png)

### Rand.
#### by block size
![](r_01bsize.png)

#### by region ratio
![](r_02region.png)

#### by No. of threads
![](r_03threads.png)

#### by region ratio in 100 threads
![](r_04regions_mthreads.png)
