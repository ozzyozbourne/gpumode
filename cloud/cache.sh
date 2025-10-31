#!/bin/bash

echo "=== Building cache demo with optimizations ==="
gcc -O2 -march=native -Wall cache_demo.c -o cache_demo -lrt

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "=== Running basic benchmark ==="
./cache_demo

echo ""
echo ""
echo "==================================================================="
echo "=== Now running with perf to see ACTUAL cache statistics ========="
echo "==================================================================="
echo ""


perf stat -e cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses,instructions,cycles ./cache_demo 2>&1 | grep -E "cache|cycles|instructions|Performance|elapsed"

echo ""
echo "==================================================================="
echo ""
echo "Perf Interpretation:"
echo "  - cache-references: Total cache accesses"
echo "  - cache-misses: When data wasn't in cache (had to go to RAM)"
echo "  - L1-dcache-loads: L1 data cache loads"
echo "  - L1-dcache-load-misses: L1 cache misses (go to L2)"
echo "  - LLC-loads: Last Level Cache (L3) loads"
echo "  - LLC-load-misses: L3 misses (go to RAM - SLOW!)"
echo ""
echo "SOA should have LOWER cache miss rates than AOS!"
echo "This is why SOA is faster despite same algorithmic complexity."
