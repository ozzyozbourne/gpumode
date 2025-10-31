#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE (1024 * 1024 * 16) // 16M elements - exceeds typical L3 cache
#define ITERATIONS 10

// Array of Structures (AOS) - poor cache locality
typedef struct {
  float x;
  float y;
  float z;
  float w;
  // padding to break the cache 
  float a, b, c, d;
  char padding[48];
} Particle_AOS;

// Structure of Arrays (SOA) - good cache locality
typedef struct {
  float *x;
  float *y;
  float *z;
  float *w;
} Particle_SOA;

static inline uint64_t get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

float sum_aos(Particle_AOS *const particles, size_t n) {
  float sum = 0.0f;
  // This will cause cache misses because we only need X,
  // but the CPU loads entire structs (X,Y,Z,W) into cache
  for (size_t i = 0; i < n; i++) { sum += particles[i].x; }
  return sum;
}

float sum_soa(Particle_SOA *const particles, const size_t n) {
  float sum = 0.0f;
  // This is cache-friendly! X values are contiguous in memory
  // When we access x[i], x[i+1], x[i+2]... are already in cache
  for (size_t i = 0; i < n; i++) { sum += particles->x[i]; }
  return sum;
}

void init_aos(Particle_AOS *const particles, const size_t n) {
  for (size_t i = 0; i < n; i++) {
    particles[i].x = (float)i;
    particles[i].y = (float)i * 2;
    particles[i].z = (float)i * 3;
    particles[i].w = (float)i * 4;
  }
}

void init_soa(Particle_SOA *const particles, const size_t n) {
  particles->x = aligned_alloc(64, n * sizeof(float));
  particles->y = aligned_alloc(64, n * sizeof(float));
  particles->z = aligned_alloc(64, n * sizeof(float));
  particles->w = aligned_alloc(64, n * sizeof(float));

  for (size_t i = 0; i < n; i++) {
    particles->x[i] = (float)i;
    particles->y[i] = (float)i * 2;
    particles->z[i] = (float)i * 3;
    particles->w[i] = (float)i * 4;
  }
}

// Do we need this ???
void free_soa(Particle_SOA *const particles) {
  free(particles->x);
  free(particles->y);
  free(particles->z);
  free(particles->w);
}

int main(void) {
  printf("Cache Locality Demonstration: SOA vs AOS\n");
  printf("=========================================\n");
  printf("Array size: %d elements (%.2f MB per array)\n\n", SIZE,
         (SIZE * sizeof(float)) / (1024.0 * 1024.0));

  printf("Initializing AOS (Array of Structures)...\n");
  Particle_AOS *aos = aligned_alloc(64, SIZE * sizeof(Particle_AOS));
  if (!aos) {
    fprintf(stderr, "Failed to allocate AOS\n");
    return 1;
  }
  init_aos(aos, SIZE);

  printf("Initializing SOA (Structure of Arrays)...\n");
  Particle_SOA soa;
  init_soa(&soa, SIZE);

  printf("\nBenchmarking...\n\n");

  uint64_t aos_total_time = 0, soa_total_time = 0;
  float aos_result = 0.0f, soa_result = 0.0f;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    aos_result = sum_aos(aos, SIZE);
    uint64_t end = get_time_ns();
    aos_total_time += (end - start);
  }

  double aos_avg_time_ms = (aos_total_time / (double)ITERATIONS) / 1e6;
  double aos_bandwidth_gb = (SIZE * sizeof(float)) / (aos_avg_time_ms / 1000.0) / 1e9;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    soa_result = sum_soa(&soa, SIZE);
    uint64_t end = get_time_ns();
    soa_total_time += (end - start);
  }

  double soa_avg_time_ms = (soa_total_time / (double)ITERATIONS) / 1e6;
  double soa_bandwidth_gb = (SIZE * sizeof(float)) / (soa_avg_time_ms / 1000.0) / 1e9;

  printf("AOS (Array of Structures):\n");
  printf("  Average time: %.3f ms\n", aos_avg_time_ms);
  printf("  Bandwidth: %.2f GB/s\n", aos_bandwidth_gb);
  printf("  Result: %.2f\n\n", aos_result);

  printf("SOA (Structure of Arrays):\n");
  printf("  Average time: %.3f ms\n", soa_avg_time_ms);
  printf("  Bandwidth: %.2f GB/s\n", soa_bandwidth_gb);
  printf("  Result: %.2f\n\n", soa_result);

  printf("Performance Comparison:\n");
  printf("  SOA is %.2fx FASTER than AOS\n", aos_avg_time_ms / soa_avg_time_ms);
  printf("  SOA achieves %.2fx HIGHER bandwidth\n\n", soa_bandwidth_gb / aos_bandwidth_gb);

  // do we need this ?????
  free(aos);
  free_soa(&soa);

  return EXIT_SUCCESS;
}
