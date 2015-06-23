#include "hash.h"
#include "hash.c.h"

void LHHash_reset(LHHash *state, unsigned long long seed)
{
  state->vtable->reset(state, seed);
}

void LHHash_update(LHHash *state, const void* input, size_t length)
{
  state->vtable->update(state, input, length);
}

unsigned long long LHHash_digest(LHHash* state)
{
  return state->vtable->digest(state);
}

LHHash *LHHash_clone(LHHash *state)
{
  return state->vtable->clone(state);
}

void LHHash_free(LHHash* state)
{
  state->vtable->free(state);
}
