# Experiment Report 01
---
Found a bug in random access of Disker at the time of measuring

## Env
- kugenuma26
- /dev/sdb1
- 31TB, RAID HDD

## Results
### Seq.
#### by address
![](s_address.png)

#### by block size
![](s_bsize.png)

### ~~Rand.~~
#### ~~by block size~~
![](r_01bsize.png)

#### ~~by region ratio~~
![](r_02region.png)

#### ~~by No. of threads~~
![](r_03threads.png)

#### ~~by region ratio in 100 threads~~
![](r_04regions_mthreads.png)