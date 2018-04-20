#!bin/bash

clear

echo "Setting up..."
rm *.txt

echo "Encoding files..."
testBin/gmprOmp -c -i testFiles/small.txt -o codedSmall
testBin/gmprOmp -c -i testFiles/medium.txt -o codedMedium
testBin/gmprOmp -c -i testFiles/big.txt -o codedBig
testBin/gmprOmp -c -i testFiles/verybig.txt -o codedVeryBig

echo "DECODING: Testing notOptimized version with small files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential_notOpt -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing notOptimized version with medium files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential_notOpt -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing notOptimized version with big files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential_notOpt -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing notOptimized version with verybig files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential_notOpt -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "-----------------------------------------------------------------------------------"

echo "DECODING: Testing sequential version with small files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing sequential version with medium files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing sequential version with big files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing sequential version with verybig files..." >> results.txt
(time for i in {1..5}; do testBin/gmprSequential -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "-----------------------------------------------------------------------------------"

export OMP_NUM_THREADS=10
export GOMP_CPU_AFFINITY="0-9"

echo "DECODING: Testing openMP 10 threads 1 cpu version with small files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 10 threads 1 cpu version with medium files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 10 threads 1 cpu version with big files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 10 threads 1 cpu version with verybig files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "-----------------------------------------------------------------------------------"

export OMP_NUM_THREADS=20
export GOMP_CPU_AFFINITY="0-19"

echo "DECODING: Testing openMP 20 threads 2 cpus version with small files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 2 cpus version with medium files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 2 cpus version with big files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 2 cpus version with verybig files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "-----------------------------------------------------------------------------------"

export OMP_NUM_THREADS=20
export GOMP_CPU_AFFINITY="0-9 20-29"

echo "DECODING: Testing openMP 20 threads 1 cpu version with small files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 1 cpu version with medium files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 1 cpu version with big files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP 20 threads 1 cpu version with verybig files..." >> results.txt
(time for i in {1..10}; do testBin/gmprOmp -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "-----------------------------------------------------------------------------------"

export OMP_NUM_THREADS=20
export GOMP_CPU_AFFINITY="0-19"

echo "DECODING: Testing openMP dynamic scheduling 20 threads 2 cpus version with small files..." >> results.txt
(time for i in {1..10}; do testBin/gmprDynamic -d -i codedSmall -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP dynamic scheduling 20 threads 2 cpus version with medium files..." >> results.txt
(time for i in {1..10}; do testBin/gmprDynamic -d -i codedMedium -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP dynamic scheduling 20 threads 2 cpus version with big files..." >> results.txt
(time for i in {1..10}; do testBin/gmprDynamic -d -i codedBig -o decoded; done) >> results.txt 2>&1

echo "DECODING: Testing openMP dynamic scheduling 20 threads 2 cpus version with verybig files..." >> results.txt
(time for i in {1..10}; do testBin/gmprDynamic -d -i codedVeryBig -o decoded; done) >> results.txt 2>&1

echo "Cleaning up..."

rm codedSmall
rm codedMedium
rm codedBig
rm codedVeryBig
rm decoded

echo "Execution ended!"
