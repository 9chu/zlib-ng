/* benchmark_adler32.cc -- benchmark adler32 variants
 * Copyright (C) 2022 Nathan Moinvaziri, Adam Stylinski
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <stdio.h>
#include <assert.h>

#include <benchmark/benchmark.h>

extern "C" {
#  include "zbuild.h"
#  include "zutil.h"
#  include "zutil_p.h"
#  include "cpu_features.h"
}

#define MAX_RANDOM_INTS (1024 * 1024)
#define MAX_RANDOM_INTS_SIZE (MAX_RANDOM_INTS * sizeof(uint32_t))

class adler32: public benchmark::Fixture {
private:
    uint32_t *random_ints;

public:
    void SetUp(const ::benchmark::State& state) {
        /* Control the alignment so that we have the best case scenario for loads. With
         * AVX512, unaligned loads can mean we're crossing a cacheline boundary at every load.
         * And while this is a realistic scenario, it makes it difficult to compare benchmark
         * to benchmark because one allocation could have been aligned perfectly for the loads
         * while the subsequent one happened to not be. This is not to be advantageous to AVX512
         * (indeed, all lesser SIMD implementations benefit from this aligned allocation), but to
         * control the _consistency_ of the results */
        random_ints = (uint32_t *)zng_alloc(MAX_RANDOM_INTS_SIZE);
        assert(random_ints != NULL);

        for (int32_t i = 0; i < MAX_RANDOM_INTS; i++) {
            random_ints[i] = rand();
        }
    }

    void Bench(benchmark::State& state, adler32_func adler32) {
        uint32_t hash = 0;

        for (auto _ : state) {
            hash = adler32(hash, (const unsigned char *)random_ints, state.range(0));
        }

        benchmark::DoNotOptimize(hash);
    }

    void TearDown(const ::benchmark::State& state) {
        zng_free(random_ints);
    }
};

#define BENCHMARK_ADLER32(name, fptr, support_flag) \
    BENCHMARK_DEFINE_F(adler32, name)(benchmark::State& state) { \
        if (!support_flag) { \
            state.SkipWithError("CPU does not support " #name); \
        } \
        Bench(state, fptr); \
    } \
    BENCHMARK_REGISTER_F(adler32, name)->Range(2048, MAX_RANDOM_INTS_SIZE);

BENCHMARK_ADLER32(c, adler32_c, 1);

#ifdef ARM_NEON_ADLER32
BENCHMARK_ADLER32(neon, adler32_neon, arm_cpu_has_neon);
#endif

#ifdef PPC_VMX_ADLER32
BENCHMARK_ADLER32(vmx, adler32_vmx, power_cpu_has_altivec);
#endif
#ifdef POWER8_VSX_ADLER32
BENCHMARK_ADLER32(power8, adler32_power8, power_cpu_has_arch_2_07);
#endif

#ifdef X86_SSSE3_ADLER32
BENCHMARK_ADLER32(ssse3, adler32_ssse3, x86_cpu_has_ssse3);
#endif
#ifdef X86_SSE41_ADLER32
BENCHMARK_ADLER32(sse41, adler32_sse41, x86_cpu_has_sse41);
#endif
#ifdef X86_AVX2_ADLER32
BENCHMARK_ADLER32(avx2, adler32_avx2, x86_cpu_has_avx2);
#endif
#ifdef X86_AVX512_ADLER32
BENCHMARK_ADLER32(avx512, adler32_avx512, x86_cpu_has_avx512);
#endif
#ifdef X86_AVX512VNNI_ADLER32
BENCHMARK_ADLER32(avx512_vnni, adler32_avx512_vnni, x86_cpu_has_avx512vnni);
#endif
