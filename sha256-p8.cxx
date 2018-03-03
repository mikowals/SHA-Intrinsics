/* sha256-p8.cxx - Power8 SHA extensions using C intrinsics  */
/*   Written and placed in public domain by Jeffrey Walton   */

/* xlC -DTEST_MAIN -qarch=pwr8 -qaltivec sha256-p8.cxx -o sha256-p8.exe  */
/* g++ -DTEST_MAIN -mcpu=power8 sha256-p8.cxx -o sha256-p8.exe           */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#if defined(__ALTIVEC__)
# include <altivec.h>
# undef vector
# undef pixel
# undef bool
#endif

#if defined(__xlc__) || defined(__xlC__)
# define TEST_SHA_XLC 1
#elif defined(__clang__)
# define TEST_SHA_CLANG 1
#elif defined(__GNUC__)
# define TEST_SHA_GCC 1
#endif

#define A16 __attribute__((aligned(16)))
typedef __vector unsigned char uint8x16_p8;
typedef __vector unsigned int  uint32x4_p8;

template <class T>
static inline
uint32x4_p8 VectorLoad32x4u(const T* data, int offset)
{
#if defined(TEST_SHA_GCC)
    return vec_vsx_ld(offset, (uint32_t*)data);
#elif defined(TEST_SHA_XLC)
    return vec_xl(offset, (uint32_t*)data);
#endif
}

template <class T>
static inline
void VectorStore32x4u(const uint32x4_p8 val, T* data, int offset)
{
#if defined(TEST_SHA_GCC)
    return vec_vsx_st(val, offset, (uint32_t*)data);
#elif defined(TEST_SHA_XLC)
    return vec_xst(val, offset, (uint32_t*)data);
#endif
}

static inline
uint32x4_p8 VectorPermute32x4(const uint32x4_p8 val, const uint8x16_p8 mask)
{
    return (uint32x4_p8)vec_perm(val, val, mask);
}

uint32x4_p8 VectorCh(const uint32x4_p8 a, const uint32x4_p8 b, const uint32x4_p8 c)
{
    // Where is vec_not in GCC???
    // return vec_xor(vec_and(a,b), vec_and(vec_not(a),c));

    const uint32x4_p8 m = {0xffffffff,0xffffffff,0xffffffff,0xffffffff};
    return vec_xor(vec_and(a,b), vec_and(vec_sub(m, a),c));
}

uint32x4_p8 VectorMaj(const uint32x4_p8 a, const uint32x4_p8 b, const uint32x4_p8 c)
{
    return vec_xor(vec_and(a, b), vec_xor(vec_and(a, c), vec_and(b, c)));
}

uint32x4_p8 Vector_sigma0(const uint32x4_p8 val)
{
#if defined(TEST_SHA_GCC)
    return __builtin_crypto_vshasigmaw(val, 0, 0);
#elif defined(TEST_SHA_XLC)
    return __vshasigmaw(val, 0, 0);
#endif
}

uint32x4_p8 Vector_sigma1(const uint32x4_p8 val)
{
#if defined(TEST_SHA_GCC)
    return __builtin_crypto_vshasigmaw(val, 0, 1);
#elif defined(TEST_SHA_XLC)
    return __vshasigmaw(val, 0, 1);
#endif
}

uint32x4_p8 VectorSigma0(const uint32x4_p8 val)
{
#if defined(TEST_SHA_GCC)
    return __builtin_crypto_vshasigmaw(val, 1, 0);
#elif defined(TEST_SHA_XLC)
    return __vshasigmaw(val, 1, 0);
#endif
}

uint32x4_p8 VectorSigma1(const uint32x4_p8 val)
{
#if defined(TEST_SHA_GCC)
    return __builtin_crypto_vshasigmaw(val, 1, 0xf);
#elif defined(TEST_SHA_XLC)
    return __vshasigmaw(val, 1, 0xf);
#endif
}

static const uint32_t K256[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

void SHA256_SCHEDULE(uint32_t W[64], const uint8_t* data)
{
#if defined(__LITTLE_ENDIAN__)
    for (unsigned int i=0; i<64; i+=4)
    {
        const uint8x16_p8 zero = {0};
        const uint8x16_p8 mask = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};
        // vec_vsx_st((uint32x4_p8)vec_perm(vec_vsx_ld(i*4, data), zero, mask), i*4, W);
        VectorStore32x4u(VectorPermute32x4(VectorLoad32x4u(data, i*4), mask), W, i*4);
    }
#else
    for (unsigned int i=0; i<64; i+=4)
    {
        // vec_vsx_st((uint32x4_p8)vec_vsx_ld(i*4, data), i*4, W);
        VectorStore32x4u(VectorLoad32x4u(data, i*4), W, i*4);
    }
#endif

#if 0
    for (unsigned int i = 16; i < 64; i+=4)
    {
        const uint32x4_p8 s0 = Vector_sigma0(VectorLoad32x4u(W, (i-15)*4));
        const uint32x4_p8 w0 = VectorLoad32x4u(W, (i-16)*4);
        const uint32x4_p8 s1 = Vector_sigma1(VectorLoad32x4u(W, (i-2)*4));
        const uint32x4_p8 w1 = VectorLoad32x4u(W, (i-7)*4);

        VectorStore32x4u(vec_add(s1, vec_add(w1, vec_add(s0, w0))), W, i*4);
    }
#endif

#if 1
    for (unsigned int i = 16; i < 64; ++i)
    {
        const uint32x4_p8 s0 = Vector_sigma0(vec_splats(W[i-15]));
        const uint32x4_p8 x0 = vec_splats(W[i-16]);
        const uint32x4_p8 s1 = Vector_sigma1(vec_splats(W[i-2]));
        const uint32x4_p8 x1 = vec_splats(W[i-7]);

        W[i] = vec_extract(
            vec_add(s1, vec_add(x1, vec_add(s0, x0))), 0
        );
    }
#endif
}

template <unsigned int R>
static inline
void SHA256_ROUND(const uint32x4_p8 k, const uint32x4_p8 w,
        uint32x4_p8& a, uint32x4_p8& b, uint32x4_p8& c, uint32x4_p8& d,
        uint32x4_p8& e, uint32x4_p8& f, uint32x4_p8& g, uint32x4_p8& h )
{
    static const int I = R;
    static const int J = I%4;

    uint32x4_p8 T1, T2;

    // T1 = h + Sigma1(e) + Ch(e,f,g) + K[t] + W[t]
    T1 = h;
    T1 = vec_add(T1, VectorSigma1(e));
    T1 = vec_add(T1, VectorCh(e,f,g));
    T1 = vec_add(T1, vec_vspltw(k, J));
    T1 = vec_add(T1, vec_vspltw(w, J));

    // T2 = Sigma0(a) + Maj(a,b,c)
    T2 = VectorSigma0(a);
    T2 = vec_add(T2, VectorMaj(a,b,c));

    h = g; g = f; f = e;
    e = vec_add(d, T1);
    d = c; c = b; b = a;
    a = vec_add(T1, T2);

    // printf("%02d: %08X\n", R, vec_extract(a, 0));
}

/* Process multiple blocks. The caller is resonsible for setting the initial */
/*  state, and the caller is responsible for padding the final block.        */
void sha256_process_p8(uint32_t state[8], const uint8_t data[], uint32_t length)
{
    uint32_t blocks = length / 64;
    if (!blocks) return;

    while (blocks--)
    {
        uint32_t W[64];
        SHA256_SCHEDULE(W, data);
        data += 64;

        const uint32x4_p8 ad = VectorLoad32x4u(state,  0);
        const uint32x4_p8 eh = VectorLoad32x4u(state, 16);
        uint32x4_p8 a,b,c,d,e,f,g,h;
        uint32x4_p8 k, w;

        a = vec_vspltw(ad, 0);
        b = vec_vspltw(ad, 1);
        c = vec_vspltw(ad, 2);
        d = vec_vspltw(ad, 3);
        e = vec_vspltw(eh, 0);
        f = vec_vspltw(eh, 1);
        g = vec_vspltw(eh, 2);
        h = vec_vspltw(eh, 3);

        k = VectorLoad32x4u(K256, 0);
        w = VectorLoad32x4u(W, 0);
        SHA256_ROUND< 0>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 1>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 2>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 3>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 16);
        w = VectorLoad32x4u(W, 16);
        SHA256_ROUND< 4>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 5>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 6>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 7>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 32);
        w = VectorLoad32x4u(W, 32);
        SHA256_ROUND< 8>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND< 9>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<10>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<11>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 48);
        w = VectorLoad32x4u(W, 48);
        SHA256_ROUND<12>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<13>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<14>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<15>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 64);
        w = VectorLoad32x4u(W, 64);
        SHA256_ROUND<16>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<17>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<18>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<19>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 80);
        w = VectorLoad32x4u(W, 80);
        SHA256_ROUND<20>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<21>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<22>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<23>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 96);
        w = VectorLoad32x4u(W, 96);
        SHA256_ROUND<24>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<25>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<26>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<27>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 112);
        w = VectorLoad32x4u(W, 112);
        SHA256_ROUND<28>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<29>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<30>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<31>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 128);
        w = VectorLoad32x4u(W, 128);
        SHA256_ROUND<32>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<33>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<34>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<35>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 144);
        w = VectorLoad32x4u(W, 144);
        SHA256_ROUND<36>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<37>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<38>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<39>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 160);
        w = VectorLoad32x4u(W, 160);
        SHA256_ROUND<40>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<41>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<42>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<43>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 176);
        w = VectorLoad32x4u(W, 176);
        SHA256_ROUND<44>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<45>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<46>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<47>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 192);
        w = VectorLoad32x4u(W, 192);
        SHA256_ROUND<48>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<49>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<50>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<51>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 208);
        w = VectorLoad32x4u(W, 208);
        SHA256_ROUND<52>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<53>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<54>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<55>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 224);
        w = VectorLoad32x4u(W, 224);
        SHA256_ROUND<56>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<57>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<58>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<59>(w,k, a,b,c,d,e,f,g,h);

        k = VectorLoad32x4u(K256, 240);
        w = VectorLoad32x4u(W, 240);
        SHA256_ROUND<60>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<61>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<62>(w,k, a,b,c,d,e,f,g,h);
        SHA256_ROUND<63>(w,k, a,b,c,d,e,f,g,h);

        state[0] += vec_extract(a, 0);
        state[1] += vec_extract(b, 0);
        state[2] += vec_extract(c, 0);
        state[3] += vec_extract(d, 0);
        state[4] += vec_extract(e, 0);
        state[5] += vec_extract(f, 0);
        state[6] += vec_extract(g, 0);
        state[7] += vec_extract(h, 0);
    }
}

#if defined(TEST_MAIN)

#include <stdio.h>
#include <string.h>
int main(int argc, char* argv[])
{
    /* empty message with padding */
    uint8_t message[64];
    memset(message, 0x00, sizeof(message));
    message[0] = 0x80;

    /* intial state */
    uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    sha256_process_p8(state, message, sizeof(message));

    /* E3B0C44298FC1C14... */
    printf("SHA256 hash of empty message: ");
    printf("%02X%02X%02X%02X%02X%02X%02X%02X...\n",
        (state[0] >> 24) & 0xFF, (state[0] >> 16) & 0xFF, (state[0] >> 8) & 0xFF, (state[0] >> 0) & 0xFF,
        (state[1] >> 24) & 0xFF, (state[1] >> 16) & 0xFF, (state[1] >> 8) & 0xFF, (state[1] >> 0) & 0xFF);

    int success = (((state[0] >> 24) & 0xFF) == 0xE3) && (((state[0] >> 16) & 0xFF) == 0xB0) &&
        (((state[0] >> 8) & 0xFF) == 0xC4) && (((state[0] >> 0) & 0xFF) == 0x42);

    if (success)
        printf("Success!\n");
    else
        printf("Failure!\n");

    return (success != 0 ? 0 : 1);
}

#endif
