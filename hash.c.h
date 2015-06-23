/* private stuff */

struct LHHashVTable {
  void (*reset)(LHHash *state, unsigned long long seed);
  void (*update)(LHHash *state, const void* input, size_t length);
  unsigned long long (*digest) (LHHash* state);
  LHHash* (*clone)(LHHash *state);
  void (*free)(LHHash* state);
};

struct LHHash_ {
  struct LHHashVTable *vtable;
};
