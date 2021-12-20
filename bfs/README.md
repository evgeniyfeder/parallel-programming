# Second task of ITMO parallel course: realize parallel BFS

Code in ```bfs.cpp```

## Instruction to run
- Download docker image from [OpenCilk repository](https://github.com/OpenCilk/infrastructure)
- ```docker load < docker-opencilk-v1.0.tar.gz```
- ```docker run -it --mount type=bind,source="$(pwd)",target=/code opencilk:v1.0 bash```
- ```clang++ -O2 -fcilkplus -DUSE_CILK_PLUS_RUNTIME -I pctl/include -I cmdline/include -I chunkedseq/include bfs.cpp```
- ```CILK_NWORKERS=4 ./a.out```
## Benchmark results for 4 CPUs and n = 500

```
Test  0
Time par = 11257802 µs
Time seq = 31020167 µs
Ratio seq/par = 2.75544
Test  1
Time par = 11336422 µs
Time seq = 30990818 µs
Ratio seq/par = 2.73374
Test  2
Time par = 11279291 µs
Time seq = 31180394 µs
Ratio seq/par = 2.76439
Test  3
Time par = 11433773 µs
Time seq = 31087350 µs
Ratio seq/par = 2.71891
Test  4
Time par = 11315059 µs
Time seq = 31034883 µs
Ratio seq/par = 2.74279
===================
Average time par = 11324469 µs
Average time seq = 31062722 µs
Ratio seq/par = 2.74297
```

