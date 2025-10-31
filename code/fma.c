#include <immintrin.h>
#include <stdio.h>

int main() {
    // 8 floats packed into one 256-bit YMM register
    __m256 a = _mm256_set1_ps(2.0f);  // all elements = 2.0
    __m256 b = _mm256_set1_ps(3.0f);  // all elements = 3.0
    __m256 c = _mm256_set1_ps(4.0f);  // all elements = 4.0

    // Perform FMA: (a * b) + c
    __m256 r = _mm256_fmadd_ps(a, b, c);

    // Store and print results
    float out[8];
    _mm256_storeu_ps(out, r);

    for (size_t i = 0; i < 8; i++) { printf("r[%ld] = %f\n", i, out[i]); }

    return 0;
}
