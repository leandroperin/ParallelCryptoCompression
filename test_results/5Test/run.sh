echo "Setting up"

echo "COMPACT" >> results.txt
export GOMP_CPU_AFFINITY="0-9"

echo "Testing 1 thread" >> results.txt
export OMP_NUM_THREADS=1
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 2 threads" >> results.txt
export OMP_NUM_THREADS=2
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 3 threads" >> results.txt
export OMP_NUM_THREADS=3
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 4 threads" >> results.txt
export OMP_NUM_THREADS=4
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 5 threads" >> results.txt
export OMP_NUM_THREADS=5
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 6 threads" >> results.txt
export OMP_NUM_THREADS=6
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 7 threads" >> results.txt
export OMP_NUM_THREADS=7
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 8 threads" >> results.txt
export OMP_NUM_THREADS=8
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 9 threads" >> results.txt
export OMP_NUM_THREADS=9
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 10 threads" >> results.txt
export OMP_NUM_THREADS=10
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "SCATTER" >> results.txt
export GOMP_CPU_AFFINITY="0 10 1 11 2 12 3 13 4 14"

echo "Testing 1 thread" >> results.txt
export OMP_NUM_THREADS=1
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 2 threads" >> results.txt
export OMP_NUM_THREADS=2
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 3 threads" >> results.txt
export OMP_NUM_THREADS=3
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 4 threads" >> results.txt
export OMP_NUM_THREADS=4
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 5 threads" >> results.txt
export OMP_NUM_THREADS=5
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 6 threads" >> results.txt
export OMP_NUM_THREADS=6
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 7 threads" >> results.txt
export OMP_NUM_THREADS=7
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 8 threads" >> results.txt
export OMP_NUM_THREADS=8
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 9 threads" >> results.txt
export OMP_NUM_THREADS=9
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1

echo "Testing 10 threads" >> results.txt
export OMP_NUM_THREADS=10
(for i in {1..10}; do ./gmpr; done) >> results.txt 2>&1
