# OS
Laboratory works for the "Operating Systems" course in the 5th semester at MAI.  
Tutor/Accepted by: Titov Yuri Pavlovich  

## Laboratory work № 4. Multithreading
To run knn.c with different number of threads:
```
gcc -o knn knn.c -lpthread -lm
./knn {number of threads}
```

Bash script to run program with 1, 2, 4, 8, 16, 32, 64 threads 100 times.  
This script also creates .txt files with execution times.
To run it:
```
chmod +x run.sh
./run.sh
```
