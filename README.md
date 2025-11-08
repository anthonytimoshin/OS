# OS
Laboratory works for the "Operating Systems" course in the 5th semester at MAI.  
Tutor/Accepted by: Titov Yuri Pavlovich  

## [Laboratory work № 4. Multithreading](https://github.com/anthonytimoshin/OS/tree/dcd54d815b619a1010a400cf74f117172ad80d48/LR4)
To run [knn.c](https://github.com/anthonytimoshin/OS/blob/dcd54d815b619a1010a400cf74f117172ad80d48/LR4/knn.c) with different number of threads:
```
gcc -o knn knn.c -lpthread -lm
./knn {number of threads}
```

Bash script [run.sh](https://github.com/anthonytimoshin/OS/blob/aa21e362e4d0e1db99032de9ce10ace0887026b6/LR4/run.sh) to run program with 1, 2, 4, 8, 16, 32, 64 threads 100 times to collect statistics.  
This script also creates .txt files with execution times.
To run it:
```
chmod +x run.sh
./run.sh
```

There is also a Python program [analyzer.py](https://github.com/anthonytimoshin/OS/blob/3acca5001a456482bab2cc82358e1b8e175d058a/LR4/analyzer.py) that analyzes the execution times of a KNN program and plots a 95% confidence interval for each number of threads.   
Educational set is 100000 elements, test set is 1000 elements. Apple M1 chip has 8 cores: 4 productive and 4 energy-efficient.  
As a result we have:

| Threads | Average value | 95% confidence interval |
| --- | --- | --- |
| 1 | 6.3341 | [6.3244, 6.3439] |
| 2 | 3.3864 | [3.3541, 3.4186] |
| 4 | 1.7824 | [1.7649, 1.8000] |
| 8 | 1.2305 | [1.2140, 1.2471] |
| 16 | 1.2349 | [1.2148, 1.2550] |
| $${\color{green}32}$$ | $${\color{green}1.1956}$$ | $${\color{green}[1.1763, 1.2149]}$$ |
| 64 | 1.4096 | [1.3876, 1.4316] |
