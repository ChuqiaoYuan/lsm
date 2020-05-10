# LSM
LSM tree implementation for Harvard CS265 project.

## Executing
To compile the programs, run the following command lines.
```
gcc -O3 heap.c level.c bloom.c hashtable.c lsm.c server.c -o lsmserver -lm

gcc -O3 client.c -o client

gcc -O3 heap.c level.c bloom.c hashtable.c lsm.c parallelizedserver.c -o pthreadserver -lpthread -lm
```

To run the programs, run the folloing command lines:
```
./lsmserver 

./client test1.txt
```

or run the parallelized version by running:
```
./pthreadserver

./client test1.txt
```

## Experiments
I use workload and data generator for CS265 which can be found [here](https://bitbucket.org/HarvardDASlab/cs265-sysproj/src/master/).

There are 8 tests used for evaulating configuration knobs and parallelization in the implementation. They can be gengerated by running the following command lines.

```
./generator --puts 10000 --gets 2000 --deletes 20 --gets-misses-ratio 0.1 --gets-skewness 0.2 --seed 265 > test1.txt

./generator --puts 10000 --gets 2000 --deletes 20 --gets-misses-ratio 0.5 --gets-skewness 0.2 --seed 265 > test2.txt

./generator --puts 10000 --gets 2000 --deletes 20 --gets-misses-ratio 0.9 --gets-skewness 0.2 --seed 265 > test3.txt

./generator --puts 10000 --gets 100 --deletes 20 --gets-misses-ratio 0.1 --gets-skewness 0.2 --seed 265 > test4.txt

./generator --puts 10000 --gets 100 --deletes 20 --gets-misses-ratio 0.5 --gets-skewness 0.2 --seed 265 > test5.txt

./generator --puts 10000 --gets 100 --deletes 20 --gets-misses-ratio 0.9 --gets-skewness 0.2 --seed 265 > test6.txt

./generator --puts 10000 --ranges 100 --gaussian-ranges --seed 265 > test7.txt

./generator --puts 10000 --ranges 100 --uniform-ranges --seed 265 > test8.txt

```


## References
https://drewdevault.com/2016/04/12/How-to-write-a-better-bloom-filter-in-C.html

https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/

https://dzone.com/articles/parallel-tcpip-socket-server-with-multi-threading

https://github.com/mbrossard/threadpool/blob/master/src/threadpool.c
