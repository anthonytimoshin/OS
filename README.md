# OS
Laboratory works for the "Operating Systems" course in the 5th semester at MAI.  
Tutor/Accepted by: Titov Yuri Pavlovich  

## [Laboratory work № 4. Multithreading](LR4)
To run [knn.c](LR4/knn.c) with different number of threads:
```
gcc -o knn knn.c -lpthread -lm
./knn {number of threads}
```
Simple knn algorithm realization without printing output to reach CPU-bound [knn_simple.c](LR4/knn_simple.c)  
To run: 
```
gcc -o knn knn_simple.c -lpthread -lm
./knn {number of threads}
```
Bash script [run.sh](LR4/run.sh) to run [knn_simple.c](LR4/knn_simple.c) with 1, 2, 4, 8, 16, 32, 64 threads 100 times to collect statistics.  
This script also creates .txt files with execution times.
To run it:
```
chmod +x run.sh
./run.sh
```

There is also a Python program [analyzer.py](LR4/analyzer.py) that analyzes the execution times of a KNN program and plots a 95% confidence interval for each number of threads.   
Educational set is 100000 elements, test set is 1000 elements. Apple M1 chip has 8 cores: 4 productive and 4 energy-efficient.  

As a result we have:

| Threads | Average value | 95% confidence interval |
| --- | --- | --- |
| 1 | 6.3341 | [6.3244; 6.3439] |
| 2 | 3.3864 | [3.3541; 3.4186] |
| 4 | 1.7824 | [1.7649; 1.8000] |
| 8 | 1.2305 | [1.2140; 1.2471] |
| 16 | 1.2349 | [1.2148; 1.2550] |
| **32** | **1.1956** | **[1.1763; 1.2149]** |
| 64 | 1.4096 | [1.3876; 1.4316] |

The shortest execution time was expected to be with an 8-thread configuration. This is because increasing the number of threads requires more time to create them and distribute the work among them. Therefore, the overhead exceeds the benefit of parallelization. However, as we can see, the shortest execution time is achieved with a 32-thread configuration. Firstly, this is because the program is not 100% CPU-bound. Secondly, the educational and test sets are quite large, and parallelization offsets the overhead of thread creation. Also, with smaller data sets, the load is distributed more evenly. However, increasing the number of threads to 64 results in an increase in execution time, which is due to thread competition and even greater overhead.
