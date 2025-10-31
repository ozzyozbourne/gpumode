#include <immintrin.h>
#include <stdio.h>

int main() {
    // Two vectors of 8 floats
    __m256 a = _mm256_setr_ps(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
    __m256 b = _mm256_setr_ps(10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f);
    
    // Add them: r = a + b (all 8 operations at once!)
    __m256 r = _mm256_add_ps(a, b);

     float a_array[8], b_array[8], out[8];

    _mm256_storeu_ps(a_array, a);
    _mm256_storeu_ps(b_array, b);   
    _mm256_storeu_ps(out, r);
    
    printf("Vector addition:\n");
    for (int i = 0; i < 8; i++) { printf("%.1f + %.1f = %.1f\n", a_array[i], b_array[i], out[i]); }
    
    return 0;
}
