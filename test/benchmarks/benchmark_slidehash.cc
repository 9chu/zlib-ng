/* benchmark_slidehash.cc -- benchmark slide_hash variants
 * Copyright (C) 2022 Adam Stylinski, Nathan Moinvaziri
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <limits.h>

#include <benchmark/benchmark.h>

extern "C" {
#  include "zbuild.h"
#  include "zutil.h"
#  include "zutil_p.h"
#  include "deflate.h"
#  include "cpu_features.h"
}

#define MAX_RANDOM_INTS 32768

class slide_hash: public benchmark::Fixture {
private:
    uint16_t *l0;
    uint16_t *l1;
    deflate_state *s_g;

public:
    void SetUp(const ::benchmark::State& state) {
        l0 = (uint16_t *)zng_alloc(HASH_SIZE * sizeof(uint16_t));

        for (int32_t i = 0; i < HASH_SIZE; i++) {
            l0[i] = rand();
        }

        l1 = (uint16_t *)zng_alloc(MAX_RANDOM_INTS * sizeof(uint16_t));

        for (int32_t i = 0; i < MAX_RANDOM_INTS; i++) {
            l1[i] = rand();
        }

        deflate_state *s = (deflate_state*)malloc(sizeof(deflate_state));
        s->head = l0;
        s->prev = l1;
        s_g = s;
    }

    void Bench(benchmark::State& state, slide_hash_func slide_hash) {
        s_g->w_size = (uint32_t)state.range(0);

        for (auto _ : state) {
            slide_hash(s_g);
            benchmark::DoNotOptimize(s_g);
        }
    }

    void TearDown(const ::benchmark::State& state) {
        zng_free(l0);
        zng_free(l1);
    }
};

#define BENCHMARK_SLIDEHASH(name, fptr, support_flag) \
    BENCHMARK_DEFINE_F(slide_hash, name)(benchmark::State& state) { \
        if (!support_flag) { \
            state.SkipWithError("CPU does not support " #name); \
        } \
        Bench(state, fptr); \
    } \
    BENCHMARK_REGISTER_F(slide_hash, name)->RangeMultiplier(2)->Range(1024, MAX_RANDOM_INTS);

BENCHMARK_SLIDEHASH(c, slide_hash_c, 1);

#ifdef ARM_NEON_SLIDEHASH
BENCHMARK_SLIDEHASH(neon, slide_hash_neon, arm_cpu_has_neon);
#endif
#ifdef POWER8_VSX_SLIDEHASH
BENCHMARK_SLIDEHASH(power8, slide_hash_power8, power_cpu_has_arch_2_07);
#endif
#ifdef PPC_VMX_SLIDEHASH
BENCHMARK_SLIDEHASH(vmx, slide_hash_vmx, power_cpu_has_altivec);
#endif

#ifdef X86_SSE2
BENCHMARK_SLIDEHASH(sse2, slide_hash_sse2, x86_cpu_has_sse2);
#endif
#ifdef X86_AVX2
BENCHMARK_SLIDEHASH(avx2, slide_hash_avx2, x86_cpu_has_avx2);
#endif
