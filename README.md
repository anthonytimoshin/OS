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

There is also a Python program that analyzes the execution times of a KNN program and plots a 95% confidence interval for each number of threads.
