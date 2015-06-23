#include <stddef.h>   /* size_t */
#include <string.h>

#include "hash.h"
#include "hash.c.h"

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   // C99
# include <stdint.h>
typedef uint8_t  BYTE;
typedef uint16_t U16;
typedef uint32_t U32;
typedef  int32_t S32;
typedef uint64_t U64;
#else
typedef unsigned char      BYTE;
typedef unsigned short     U16;
typedef unsigned int       U32;
typedef   signed int       S32;
typedef unsigned long long U64;
#endif

typedef struct
{
  struct LHHashVTable *vtable;
  U64 total_len;
  U64 seed;
  U64 v1;
  U64 v2;
  U64 v3;
  U64 v4;
  U32 memsize;
  char memory[32];
} XXH64_state_t;

LHHash* LHXXH64_new(void);
static void XXH64_free(LHHash* statePtr);
static void XXH64_reset(LHHash* statePtr, unsigned long long seed);
static void XXH64_update(LHHash* statePtr, const void* input, size_t length);
static unsigned long long XXH64_digest(LHHash* statePtr);

#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#  define XXH_USE_UNALIGNED_ACCESS 1
#endif

#define XXH_FORCE_NATIVE_FORMAT 0

//**************************************
// Compiler Specific Options
//**************************************
// Disable some Visual warning messages
#ifdef _MSC_VER  // Visual Studio
#  pragma warning(disable : 4127)      // disable: C4127: conditional expression is constant
#endif

#ifdef _MSC_VER    // Visual Studio
#  define FORCE_INLINE static __forceinline
#else
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif

#include <stdlib.h>

//**************************************
// Basic Types
//**************************************
#if defined(__GNUC__)  && !defined(XXH_USE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  ifdef __IBMC__
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif

typedef struct _U32_S
{
  U32 v;
} _PACKED U32_S;
typedef struct _U64_S
{
  U64 v;
} _PACKED U64_S;

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A32(x) (((U32_S *)(x))->v)
#define A64(x) (((U64_S *)(x))->v)

//***************************************
// Compiler-specific Functions and Macros
//***************************************
#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

// Note : although _rotl exists for minGW (GCC under windows), performance seems poor
#if defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif

#if defined(_MSC_VER)     // Visual Studio
#  define XXH_swap32 _byteswap_ulong
#  define XXH_swap64 _byteswap_uint64
#elif GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#  define XXH_swap64 __builtin_bswap64
#else
static inline U32 XXH_swap32 (U32 x)
{
  return  ((x << 24) & 0xff000000 ) |
    ((x <<  8) & 0x00ff0000 ) |
    ((x >>  8) & 0x0000ff00 ) |
    ((x >> 24) & 0x000000ff );
}
static inline U64 XXH_swap64 (U64 x)
{
  return  ((x << 56) & 0xff00000000000000ULL) |
    ((x << 40) & 0x00ff000000000000ULL) |
    ((x << 24) & 0x0000ff0000000000ULL) |
    ((x << 8)  & 0x000000ff00000000ULL) |
    ((x >> 8)  & 0x00000000ff000000ULL) |
    ((x >> 24) & 0x0000000000ff0000ULL) |
    ((x >> 40) & 0x000000000000ff00ULL) |
    ((x >> 56) & 0x00000000000000ffULL);
}
#endif


//**************************************
// Constants
//**************************************
#define PRIME32_1   2654435761U
#define PRIME32_2   2246822519U
#define PRIME32_3   3266489917U
#define PRIME32_4    668265263U
#define PRIME32_5    374761393U

#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL

//**************************************
// Architecture Macros
//**************************************
typedef enum { XXH_bigEndian=0, XXH_littleEndian=1 } XXH_endianess;
#ifndef XXH_CPU_LITTLE_ENDIAN   // It is possible to define XXH_CPU_LITTLE_ENDIAN externally, for example using a compiler switch
static const int one = 1;
#   define XXH_CPU_LITTLE_ENDIAN   (*(char*)(&one))
#endif


//**************************************
// Macros
//**************************************
#define XXH_STATIC_ASSERT(c)   { enum { XXH_static_assert = 1/(!!(c)) }; }    // use only *after* variable declarations


//****************************
// Memory reads
//****************************
typedef enum { XXH_aligned, XXH_unaligned } XXH_alignment;

static inline U32 XXH_readLE32_align(const U32* ptr, XXH_endianess endian, XXH_alignment align)
{
  if (align==XXH_unaligned)
    return endian==XXH_littleEndian ? A32(ptr) : XXH_swap32(A32(ptr));
  else
    return endian==XXH_littleEndian ? *ptr : XXH_swap32(*ptr);
}

static inline U32 XXH_readLE32(const U32* ptr, XXH_endianess endian)
{
  return XXH_readLE32_align(ptr, endian, XXH_unaligned);
}

static inline U64 XXH_readLE64_align(const U64* ptr, XXH_endianess endian, XXH_alignment align)
{
  if (align==XXH_unaligned)
    return endian==XXH_littleEndian ? A64(ptr) : XXH_swap64(A64(ptr));
  else
    return endian==XXH_littleEndian ? *ptr : XXH_swap64(*ptr);
}

static inline U64 XXH_readLE64(const U64* ptr, XXH_endianess endian)
{
  return XXH_readLE64_align(ptr, endian, XXH_unaligned);
}


/****************************************************
 *  Advanced Hash Functions
 ****************************************************/

static void XXH64_free(LHHash* statePtr)
{
  free(statePtr);
};


/*** Hash feed ***/

static void XXH64_reset(LHHash *state_in, unsigned long long seed)
{
  XXH64_state_t* state = (XXH64_state_t*)state_in;
  state->seed = seed;
  state->v1 = seed + PRIME64_1 + PRIME64_2;
  state->v2 = seed + PRIME64_2;
  state->v3 = seed + 0;
  state->v4 = seed - PRIME64_1;
  state->total_len = 0;
  state->memsize = 0;
}

static void XXH64_update_endian (XXH64_state_t* state, const void* input, size_t len, XXH_endianess endian)
{
  const BYTE* p = (const BYTE*)input;
  const BYTE* const bEnd = p + len;

  state->total_len += len;

  if (state->memsize + len < 32)   // fill in tmp buffer
  {
    memcpy(state->memory + state->memsize, input, len);
    state->memsize += (U32)len;
    return;
  }

  if (state->memsize)   // some data left from previous update
  {
    memcpy(state->memory + state->memsize, input, 32-state->memsize);
    {
      const U64* p64 = (const U64*)state->memory;
      state->v1 += XXH_readLE64(p64, endian) * PRIME64_2;
      state->v1 = XXH_rotl64(state->v1, 31);
      state->v1 *= PRIME64_1;
      p64++;
      state->v2 += XXH_readLE64(p64, endian) * PRIME64_2;
      state->v2 = XXH_rotl64(state->v2, 31);
      state->v2 *= PRIME64_1;
      p64++;
      state->v3 += XXH_readLE64(p64, endian) * PRIME64_2;
      state->v3 = XXH_rotl64(state->v3, 31);
      state->v3 *= PRIME64_1;
      p64++;
      state->v4 += XXH_readLE64(p64, endian) * PRIME64_2;
      state->v4 = XXH_rotl64(state->v4, 31);
      state->v4 *= PRIME64_1;
      p64++;
    }
    p += 32-state->memsize;
    state->memsize = 0;
  }

  if (p+32 <= bEnd)
  {
    const BYTE* const limit = bEnd - 32;
    U64 v1 = state->v1;
    U64 v2 = state->v2;
    U64 v3 = state->v3;
    U64 v4 = state->v4;

    do
    {
      v1 += XXH_readLE64((const U64*)p, endian) * PRIME64_2;
      v1 = XXH_rotl64(v1, 31);
      v1 *= PRIME64_1;
      p+=8;
      v2 += XXH_readLE64((const U64*)p, endian) * PRIME64_2;
      v2 = XXH_rotl64(v2, 31);
      v2 *= PRIME64_1;
      p+=8;
      v3 += XXH_readLE64((const U64*)p, endian) * PRIME64_2;
      v3 = XXH_rotl64(v3, 31);
      v3 *= PRIME64_1;
      p+=8;
      v4 += XXH_readLE64((const U64*)p, endian) * PRIME64_2;
      v4 = XXH_rotl64(v4, 31);
      v4 *= PRIME64_1;
      p+=8;
    }
    while (p<=limit);

    state->v1 = v1;
    state->v2 = v2;
    state->v3 = v3;
    state->v4 = v4;
  }

  if (p < bEnd)
  {
    memcpy(state->memory, p, bEnd-p);
    state->memsize = (int)(bEnd-p);
  }
}

static void XXH64_update(LHHash* state_in, const void* input, size_t len)
{
  XXH64_state_t* state = (XXH64_state_t*)state_in;
  XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

  if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
    return XXH64_update_endian(state, input, len, XXH_littleEndian);
  else
    return XXH64_update_endian(state, input, len, XXH_bigEndian);
}



static inline U64 XXH64_digest_endian(const XXH64_state_t* state, XXH_endianess endian)
{
  const BYTE * p = (const BYTE*)state->memory;
  BYTE* bEnd = (BYTE*)state->memory + state->memsize;
  U64 h64;

  if (state->total_len >= 32)
  {
    U64 v1 = state->v1;
    U64 v2 = state->v2;
    U64 v3 = state->v3;
    U64 v4 = state->v4;

    h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

    v1 *= PRIME64_2;
    v1 = XXH_rotl64(v1, 31);
    v1 *= PRIME64_1;
    h64 ^= v1;
    h64 = h64*PRIME64_1 + PRIME64_4;

    v2 *= PRIME64_2;
    v2 = XXH_rotl64(v2, 31);
    v2 *= PRIME64_1;
    h64 ^= v2;
    h64 = h64*PRIME64_1 + PRIME64_4;

    v3 *= PRIME64_2;
    v3 = XXH_rotl64(v3, 31);
    v3 *= PRIME64_1;
    h64 ^= v3;
    h64 = h64*PRIME64_1 + PRIME64_4;

    v4 *= PRIME64_2;
    v4 = XXH_rotl64(v4, 31);
    v4 *= PRIME64_1;
    h64 ^= v4;
    h64 = h64*PRIME64_1 + PRIME64_4;
  }
  else
  {
    h64  = state->seed + PRIME64_5;
  }

  h64 += (U64) state->total_len;

  while (p+8<=bEnd)
  {
    U64 k1 = XXH_readLE64((const U64*)p, endian);
    k1 *= PRIME64_2;
    k1 = XXH_rotl64(k1,31);
    k1 *= PRIME64_1;
    h64 ^= k1;
    h64 = XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
    p+=8;
  }

  if (p+4<=bEnd)
  {
    h64 ^= (U64)(XXH_readLE32((const U32*)p, endian)) * PRIME64_1;
    h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
    p+=4;
  }

  while (p<bEnd)
  {
    h64 ^= (*p) * PRIME64_5;
    h64 = XXH_rotl64(h64, 11) * PRIME64_1;
    p++;
  }

  h64 ^= h64 >> 33;
  h64 *= PRIME64_2;
  h64 ^= h64 >> 29;
  h64 *= PRIME64_3;
  h64 ^= h64 >> 32;

  return h64;
}


static unsigned long long XXH64_digest(LHHash* state_in)
{
  XXH64_state_t *state = (XXH64_state_t*)state_in;
  XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

  if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
    return XXH64_digest_endian(state, XXH_littleEndian);
  else
    return XXH64_digest_endian(state, XXH_bigEndian);
}

static LHHash* XXH64_clone(LHHash *state)
{
  LHHash *newstate = (LHHash*)malloc(sizeof(XXH64_state_t));
  memcpy(newstate, state, sizeof(XXH64_state_t));
  return newstate;
}

static struct LHHashVTable LHXXH64VTable = {
  XXH64_reset,
  XXH64_update,
  XXH64_digest,
  XXH64_clone,
  XXH64_free
};

LHHash* LHXXH64_new(void)
{
  LHHash *state = (LHHash*)malloc(sizeof(XXH64_state_t));
  if(state) {
    state->vtable = &LHXXH64VTable;
  }
  return state;
}
