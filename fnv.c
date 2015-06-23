#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "hash.c.h"

#define FNV_64_PRIME ((unsigned long long)0x100000001b3ULL)

typedef struct {
  struct LHHashVTable *vtable;
  unsigned long long hval;
} LHFNV64Hash;

static void FNV64_reset(LHHash* state_in, unsigned long long seed)
{
  LHFNV64Hash *state = (LHFNV64Hash*)state_in;
  state->hval = seed;
}

static void FNV64_update(LHHash* state_in, const void* buf, size_t len)
{
  LHFNV64Hash *state = (LHFNV64Hash*)state_in;
  unsigned char *bp = (unsigned char *)buf;/* start of buffer */
  unsigned char *be = bp + len;/* beyond end of buffer */
  unsigned long long hval = state->hval;

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {

    /* xor the bottom with the current octet */
    hval ^= (unsigned long long)*bp++;

    /* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
    hval += (hval << 1) + (hval << 4) + (hval << 5) +
      (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */
  }

  /* return our new hash value */
  state->hval = hval;
}

static unsigned long long FNV64_digest(LHHash* state_in)
{
  LHFNV64Hash *state = (LHFNV64Hash*)state_in;
  return state->hval;
}

static LHHash* FNV64_clone(LHHash *state)
{
  LHHash *newstate = (LHHash*)malloc(sizeof(LHFNV64Hash));
  memcpy(newstate, state, sizeof(LHFNV64Hash));
  return newstate;
}

static void FNV64_free(LHHash* state)
{
  free(state);
};

static struct LHHashVTable LHFNV64VTable = {
  FNV64_reset,
  FNV64_update,
  FNV64_digest,
  FNV64_clone,
  FNV64_free
};

LHHash* LHFNV64_new(void)
{
  LHHash *state = (LHHash*)malloc(sizeof(LHFNV64Hash));
  if(state) {
    state->vtable = &LHFNV64VTable;
  }
  return state;
}
