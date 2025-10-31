#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE (1024 * 1024 * 16) // 16M elements - exceeds typical L3 cache
#define ITERATIONS 10

typedef struct {
  float x;
  float y;
  float z;
  float w;
  // padding to break the cache
  float a, b, c, d;
  char padding[48];
} Particle_AOS;

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

float sum_aos_scalar(Particle_AOS *const particles, const size_t n) {
  float sum = 0.0f;
  for (size_t i = 0; i < n; i++) { sum += particles[i].x; }
  return sum;
}

float sum_soa_scalar(Particle_SOA *const particles, const size_t n) {
  float sum = 0.0f;
  for (size_t i = 0; i < n; i++) { sum += particles->x[i]; }
  return sum;
}

float sum_aos_simd(Particle_AOS *const particles, size_t n) {
  __m256 vec_sum = _mm256_setzero_ps();
  float sum = 0.0f;

  // Process 8 particles at a time
  // This is VERY inefficient because x values are not contiguous!
  // We need to gather scattered x values from different structs
  size_t simd_end = n - (n % 8);
  for (size_t i = 0; i < simd_end; i += 8) {
    // Manually load 8 x values from non-contiguous memory
    // This is essentially what _mm256_set_ps does, but from our structs
    __m256 vec = _mm256_set_ps(particles[i + 7].x, particles[i + 6].x,
                               particles[i + 5].x, particles[i + 4].x,
                               particles[i + 3].x, particles[i + 2].x,
                               particles[i + 1].x, particles[i + 0].x);
    vec_sum = _mm256_add_ps(vec_sum, vec);
  }

  // Horizontal sum of the vector
  float temp[8];
  _mm256_storeu_ps(temp, vec_sum);
  for (size_t j = 0; j < 8; j++) { sum += temp[j]; }

  // Handle remaining elements
  for (size_t j = 0; j < 8; j++) { sum += particles[j].x; }
  return sum;
}

float sum_soa_simd(Particle_SOA *const particles, size_t n) {
  __m256 vec_sum = _mm256_setzero_ps();
  float sum = 0.0f;
  size_t simd_end = n - (n % 8);
  // Process 8 floats at a time
  // This is VERY efficient because x values are contiguous!
  for (size_t i = 0; i < simd_end; i += 8) {
    __m256 vec = _mm256_loadu_ps(&particles->x[i]);
    vec_sum = _mm256_add_ps(vec_sum, vec);
  }

  // Horizontal sum of the vector
  float temp[8];
  _mm256_storeu_ps(temp, vec_sum);

  for (size_t j = 0; j < 8; j++) { sum += temp[j]; }

  // Handle remaining elements
  for (size_t j = simd_end; j < n; j++) { sum += particles->x[j]; }
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

void free_soa(Particle_SOA *const particles) {
  free(particles->x);
  free(particles->y);
  free(particles->z);
  free(particles->w);
}

int main(void) {
  printf("Cache Locality + SIMD Demonstration: SOA vs AOS\n");
  printf("================================================\n");
  printf("Array size: %d elements (%.2f MB per array)\n", SIZE, (SIZE * sizeof(float)) / (1024.0 * 1024.0));
  printf("SIMD: AVX2 (8 floats per operation)\n\n");

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

  uint64_t aos_scalar_total_time = 0, aos_simd_total_time = 0;
  float aos_scalar_result = 0.0f, aos_simd_result = 0.0f;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    aos_scalar_result = sum_aos_scalar(aos, SIZE);
    uint64_t end = get_time_ns();
    aos_scalar_total_time += (end - start);
  }

  double aos_scalar_avg_time_ms = (aos_scalar_total_time / (double)ITERATIONS) / 1e6;
  double aos_scalar_bandwidth_gb = (SIZE * sizeof(float)) / (aos_scalar_avg_time_ms / 1000.0) / 1e9;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    aos_simd_result = sum_aos_simd(aos, SIZE);
    uint64_t end = get_time_ns();
    aos_simd_total_time += (end - start);
  }

  double aos_simd_avg_time_ms = (aos_simd_total_time / (double)ITERATIONS) / 1e6;
  double aos_simd_bandwidth_gb = (SIZE * sizeof(float)) / (aos_simd_avg_time_ms / 1000.0) / 1e9;

  uint64_t soa_scalar_total_time = 0, soa_simd_total_time = 0;
  float soa_scalar_result = 0.0f, soa_simd_result = 0.0f;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    soa_scalar_result = sum_soa_scalar(&soa, SIZE);
    uint64_t end = get_time_ns();
    soa_scalar_total_time += (end - start);
  }

  double soa_scalar_avg_time_ms = (soa_scalar_total_time / (double)ITERATIONS) / 1e6;
  double soa_scalar_bandwidth_gb = (SIZE * sizeof(float)) / (soa_scalar_avg_time_ms / 1000.0) / 1e9;

  for (size_t i = 0; i < ITERATIONS; i++) {
    uint64_t start = get_time_ns();
    soa_simd_result = sum_soa_simd(&soa, SIZE);
    uint64_t end = get_time_ns();
    soa_simd_total_time += (end - start);
  }

  double soa_simd_avg_time_ms = (soa_simd_total_time / (double)ITERATIONS) / 1e6;
  double soa_simd_bandwidth_gb = (SIZE * sizeof(float)) / (soa_simd_avg_time_ms / 1000.0) / 1e9;

  printf("║ AOS (Array of Structures) - SCALAR                          \n");
  printf("║   Time:      %8.3f ms                                       \n",aos_scalar_avg_time_ms);
  printf("║   Bandwidth: %8.2f GB/s                                     \n",aos_scalar_bandwidth_gb);
  printf("║   Result:    %8.2f                                          \n",aos_scalar_result);
  printf("╠═════════════════════════════════════════════════════════════\n");
  printf("║ SOA (Structure of Arrays) - SCALAR                          \n");
  printf("║   Time:      %8.3f ms                                       \n",soa_scalar_avg_time_ms);
  printf("║   Bandwidth: %8.2f GB/s                                     \n",soa_scalar_bandwidth_gb);
  printf("║   Result:    %8.2f                                          \n",soa_scalar_result);
  printf("╠═════════════════════════════════════════════════════════════\n");
  printf("║ SCALAR: SOA is %.2fx FASTER than AOS                        \n",aos_scalar_avg_time_ms / soa_scalar_avg_time_ms);




  printf("║ AOS (Array of Structures) - SIMD (AVX2)                     \n");
  printf("║   Time:      %8.3f ms                                       \n",aos_simd_avg_time_ms);
  printf("║   Bandwidth: %8.2f GB/s                                     \n",aos_simd_bandwidth_gb);
  printf("║   Result:    %8.2f                                          \n", aos_simd_result);
  printf("║   Speedup:   %.2fx over scalar                              \n",aos_scalar_avg_time_ms / aos_simd_avg_time_ms);
  printf("╠═════════════════════════════════════════════════════════════\n");
  printf("║ SOA (Structure of Arrays) - SIMD (AVX2)                     \n");
  printf("║   Time:      %8.3f ms                                       \n",soa_simd_avg_time_ms);
  printf("║   Bandwidth: %8.2f GB/s                                     \n",soa_simd_bandwidth_gb);
  printf("║   Result:    %8.2f                                          \n",soa_simd_result);
  printf("║   Speedup:   %.2fx over scalar                              \n",soa_scalar_avg_time_ms / soa_simd_avg_time_ms);
  printf("╠═════════════════════════════════════════════════════════════\n");
  printf("║ SIMD: SOA is %.2fx FASTER than AOS                          \n",aos_simd_avg_time_ms / soa_simd_avg_time_ms);

  free(aos);
  free_soa(&soa);

  return EXIT_SUCCESS;
}
