#include <stdint.h>
#include <stdlib.h>

#define FNV_32_PRIME ((uint32_t)0x01000193)

uint32_t libhash_fnv_32(const void *buf, size_t len, uint32_t hval)
{
  unsigned char *bp = (unsigned char *)buf;/* start of buffer */
  unsigned char *be = bp + len;/* beyond end of buffer */

  /*
   * FNV-1 hash each octet in the buffer
   */
  while (bp < be) {

    /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_32_PRIME;
#else
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif

    /* xor the bottom with the current octet */
    hval ^= (uint32_t)*bp++;
  }

  /* return our new hash value */
  return hval;
}

uint32_t libhash_fnv_32a(const void *buf, size_t len, uint32_t hval)
{
  unsigned char *bp = (unsigned char *)buf;/* start of buffer */
  unsigned char *be = bp + len;/* beyond end of buffer */

  /*
   * FNV-1a hash each octet in the buffer
   */
  while (bp < be) {

    /* xor the bottom with the current octet */
    hval ^= (uint32_t)*bp++;

    /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_32_PRIME;
#else
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
  }

  /* return our new hash value */
  return hval;
}

#define FNV_64_PRIME ((uint64_t)0x100000001b3ULL)

uint64_t libhash_fnv_64(const void *buf, size_t len, uint64_t hval)
{
  unsigned char *bp = (unsigned char *)buf;/* start of buffer */
  unsigned char *be = bp + len;/* beyond end of buffer */

  /*
   * FNV-1 hash each octet of the buffer
   */
  while (bp < be) {

    /* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
    hval += (hval << 1) + (hval << 4) + (hval << 5) +
      (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */

    /* xor the bottom with the current octet */
    hval ^= (uint64_t)*bp++;
  }

  /* return our new hash value */
  return hval;
}

uint64_t libhash_fnv_64a(const void *buf, size_t len, uint64_t hval)
{
  unsigned char *bp = (unsigned char *)buf;/* start of buffer */
  unsigned char *be = bp + len;/* beyond end of buffer */

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {

    /* xor the bottom with the current octet */
    hval ^= (uint64_t)*bp++;

    /* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
    hval += (hval << 1) + (hval << 4) + (hval << 5) +
      (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */
  }

  /* return our new hash value */
  return hval;
}
