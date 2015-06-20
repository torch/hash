#include <stdint.h>
#include <stdlib.h>

/* hash */
#define tommy_cast(type, value) (value)

static inline uint32_t tommy_le_uint32_read(const void* ptr)
{
  /* allow unaligned read on Intel x86 and x86_64 platforms */
#if defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_X64)
  /* defines from http://predef.sourceforge.net/ */
  return *(const uint32_t*)ptr;
#else
  const unsigned char* ptr8 = tommy_cast(const unsigned char*, ptr);
  return ptr8[0] + ((uint32_t)ptr8[1] << 8) + ((uint32_t)ptr8[2] << 16) + ((uint32_t)ptr8[3] << 24);
#endif
}

#define tommy_rot(x, k) \
  (((x) << (k)) | ((x) >> (32 - (k))))

#define tommy_mix(a, b, c) \
  do { \
    a -= c;  a ^= tommy_rot(c, 4);  c += b; \
    b -= a;  b ^= tommy_rot(a, 6);  a += c; \
    c -= b;  c ^= tommy_rot(b, 8);  b += a; \
    a -= c;  a ^= tommy_rot(c, 16);  c += b; \
    b -= a;  b ^= tommy_rot(a, 19);  a += c; \
    c -= b;  c ^= tommy_rot(b, 4);  b += a; \
  } while (0)

#define tommy_final(a, b, c) \
  do { \
    c ^= b; c -= tommy_rot(b, 14); \
    a ^= c; a -= tommy_rot(c, 11); \
    b ^= a; b -= tommy_rot(a, 25); \
    c ^= b; c -= tommy_rot(b, 16); \
    a ^= c; a -= tommy_rot(c, 4);  \
    b ^= a; b -= tommy_rot(a, 14); \
    c ^= b; c -= tommy_rot(b, 24); \
  } while (0)

uint32_t libhash_lookup3_32(const void* void_key, size_t key_len, uint32_t init_val)
{
  const unsigned char* key = tommy_cast(const unsigned char*, void_key);
  uint32_t a, b, c;

  a = b = c = 0xdeadbeef + ((uint32_t)key_len) + init_val;

  while (key_len > 12) {
    a += tommy_le_uint32_read(key + 0);
    b += tommy_le_uint32_read(key + 4);
    c += tommy_le_uint32_read(key + 8);

    tommy_mix(a, b, c);

    key_len -= 12;
    key += 12;
  }

  switch (key_len) {
  case 0 :
    return c; /* used only when called with a zero length */
  case 12 :
    c += tommy_le_uint32_read(key + 8);
    b += tommy_le_uint32_read(key + 4);
    a += tommy_le_uint32_read(key + 0);
    break;
  case 11 : c += ((uint32_t)key[10]) << 16;
  case 10 : c += ((uint32_t)key[9]) << 8;
  case 9 : c += key[8];
  case 8 :
    b += tommy_le_uint32_read(key + 4);
    a += tommy_le_uint32_read(key + 0);
    break;
  case 7 : b += ((uint32_t)key[6]) << 16;
  case 6 : b += ((uint32_t)key[5]) << 8;
  case 5 : b += key[4];
  case 4 :
    a += tommy_le_uint32_read(key + 0);
    break;
  case 3 : a += ((uint32_t)key[2]) << 16;
  case 2 : a += ((uint32_t)key[1]) << 8;
  case 1 : a += key[0];
  }

  tommy_final(a, b, c);

  return c;
}

uint64_t libhash_lookup3_64(const void* void_key, size_t key_len, uint64_t init_val)
{
  const unsigned char* key = tommy_cast(const unsigned char*, void_key);
  uint32_t a, b, c;

  a = b = c = 0xdeadbeef + ((uint32_t)key_len) + (init_val & 0xffffffff);
  c += init_val >> 32;

  while (key_len > 12) {
    a += tommy_le_uint32_read(key + 0);
    b += tommy_le_uint32_read(key + 4);
    c += tommy_le_uint32_read(key + 8);

    tommy_mix(a, b, c);

    key_len -= 12;
    key += 12;
  }

  switch (key_len) {
  case 0 :
    return c + ((uint64_t)b << 32); /* used only when called with a zero length */
  case 12 :
    c += tommy_le_uint32_read(key + 8);
    b += tommy_le_uint32_read(key + 4);
    a += tommy_le_uint32_read(key + 0);
    break;
  case 11 : c += ((uint32_t)key[10]) << 16;
  case 10 : c += ((uint32_t)key[9]) << 8;
  case 9 : c += key[8];
  case 8 :
    b += tommy_le_uint32_read(key + 4);
    a += tommy_le_uint32_read(key + 0);
    break;
  case 7 : b += ((uint32_t)key[6]) << 16;
  case 6 : b += ((uint32_t)key[5]) << 8;
  case 5 : b += key[4];
  case 4 :
    a += tommy_le_uint32_read(key + 0);
    break;
  case 3 : a += ((uint32_t)key[2]) << 16;
  case 2 : a += ((uint32_t)key[1]) << 8;
  case 1 : a += key[0];
  }

  tommy_final(a, b, c);

  return c + ((uint64_t)b << 32);
}
