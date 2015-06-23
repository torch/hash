#ifndef LIBHASH_HASH_INC
#define LIBHASH_HASH_INC

#include <stddef.h>   /* size_t */

typedef struct LHHash_ LHHash;

LHHash* LHXXH64_new(void);
LHHash* LHFNV64_new(void);

void LHHash_reset(LHHash *state, unsigned long long seed);
void LHHash_update(LHHash *state, const void* input, size_t length);
unsigned long long LHHash_digest(LHHash* state);
LHHash* LHHash_clone(LHHash *state);
void LHHash_free(LHHash* state);

#endif
