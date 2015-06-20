#include <lua.h>
#include <lauxlib.h>

#include "luaT.h"
#include "TH.h"

typedef uint32_t (*libhash_hashfunc32_t)(const void *data, size_t len, uint32_t seed);
typedef uint64_t (*libhash_hashfunc64_t)(const void *data, size_t len, uint64_t seed);

uint32_t libhash_fnv_32(const void *buf, size_t len, uint32_t hval);
uint32_t libhash_fnv_32a(const void *buf, size_t len, uint32_t hval);
uint64_t libhash_fnv_64(const void *buf, size_t len, uint64_t hval);
uint64_t libhash_fnv_64a(const void *buf, size_t len, uint64_t hval);

uint32_t libhash_lookup3_32(const void* void_key, size_t key_len, uint32_t init_val);
uint64_t libhash_lookup3_64(const void* void_key, size_t key_len, uint64_t init_val);

uint32_t libhash_murmur3_32(const void *key, size_t len, uint32_t seed);

#define IMPLEMENT_THTENSOR_HASH_CLOSURE(BITS, TYPE, CTYPE)              \
  uint##BITS##_t TH##TYPE##Tensor_hash##BITS(                           \
    TH##TYPE##Tensor *tensor,                                           \
    uint##BITS##_t (*hashfunc)(const void *data, size_t len, uint##BITS##_t seed), \
    uint##BITS##_t seed)                                                \
  {                                                                     \
    TH_TENSOR_APPLY(CTYPE, tensor,                                      \
                    seed = hashfunc(tensor_data, tensor_size, seed); break;); \
    return seed;                                                        \
  }                                                                     \

#define IMPLEMENT_THTENSOR_HASH(BITS, TYPE, CTYPE, HASHNAME)  \
  uint##BITS##_t TH##TYPE##Tensor_##HASHNAME##Hash##BITS(TH##TYPE##Tensor *tensor, uint##BITS##_t seed) \
  {                                                                     \
    TH_TENSOR_APPLY(CTYPE, tensor,                                      \
                    seed = libhash_##HASHNAME##_##BITS(tensor_data, tensor_size, seed); break;); \
    return seed;                                                        \
  }                                                                     \


IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Byte, unsigned char);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Char, char);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Short, short);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Int, int);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Long, long);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Float, float);
IMPLEMENT_THTENSOR_HASH_CLOSURE(32, Double, double);

IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Byte, unsigned char);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Char, char);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Short, short);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Int, int);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Long, long);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Float, float);
IMPLEMENT_THTENSOR_HASH_CLOSURE(64, Double, double);

#define IMPLEMENT_ALL_THTENSOR_HASH32(HASHNAME)                      \
  IMPLEMENT_THTENSOR_HASH(32, Byte, unsigned char, HASHNAME);        \
  IMPLEMENT_THTENSOR_HASH(32, Char, char, HASHNAME);                 \
  IMPLEMENT_THTENSOR_HASH(32, Short, short, HASHNAME);               \
  IMPLEMENT_THTENSOR_HASH(32, Int, int, HASHNAME);                   \
  IMPLEMENT_THTENSOR_HASH(32, Long, long, HASHNAME);                 \
  IMPLEMENT_THTENSOR_HASH(32, Float, float, HASHNAME);               \
  IMPLEMENT_THTENSOR_HASH(32, Double, double, HASHNAME);

#define IMPLEMENT_ALL_THTENSOR_HASH64(HASHNAME)                      \
  IMPLEMENT_THTENSOR_HASH(64, Byte, unsigned char, HASHNAME);        \
  IMPLEMENT_THTENSOR_HASH(64, Char, char, HASHNAME);                 \
  IMPLEMENT_THTENSOR_HASH(64, Short, short, HASHNAME);               \
  IMPLEMENT_THTENSOR_HASH(64, Int, int, HASHNAME);                   \
  IMPLEMENT_THTENSOR_HASH(64, Long, long, HASHNAME);                 \
  IMPLEMENT_THTENSOR_HASH(64, Float, float, HASHNAME);               \
  IMPLEMENT_THTENSOR_HASH(64, Double, double, HASHNAME);

IMPLEMENT_ALL_THTENSOR_HASH32(lookup3);
IMPLEMENT_ALL_THTENSOR_HASH64(lookup3);


static libhash_hashfunc32_t hashname2hashfunc32(lua_State *L, const char *hashname)
{
  libhash_hashfunc32_t hashfunc;

  if(!strcmp(hashname, "lookup3"))
    hashfunc = libhash_lookup3_32;
  else if(!strcmp(hashname, "murmur3"))
    hashfunc = libhash_murmur3_32;
  else if(!strcmp(hashname, "fnv"))
    hashfunc = libhash_fnv_32;
  else if(!strcmp(hashname, "fnv-a"))
    hashfunc = libhash_fnv_32a;
  else
    luaL_error(L, "invalid hash type (lookup3, murmur3, fnv, or fnv-a expected)");
  return hashfunc;
}

static libhash_hashfunc64_t hashname2hashfunc64(lua_State *L, const char *hashname)
{
  libhash_hashfunc64_t hashfunc;

  if(!strcmp(hashname, "lookup3"))
    hashfunc = libhash_lookup3_64;
  else if(!strcmp(hashname, "fnv"))
    hashfunc = libhash_fnv_64;
  else if(!strcmp(hashname, "fnv-a"))
    hashfunc = libhash_fnv_64a;
  else
    luaL_error(L, "invalid hash type (lookup3, murmur3, fnv, or fnv-a expected)");
  return hashfunc;
}

#define IMPLEMENT_HASH(BITS)                                            \
  static int libhash_hash##BITS(lua_State *L)                           \
  {                                                                     \
    int narg = lua_gettop(L);                                           \
    const char* hashname = luaL_optstring(L, 2, "lookup3");             \
    uint##BITS##_t seed = (uint##BITS##_t)luaL_optlong(L, 3, 0);        \
    uint##BITS##_t hash = 0;                                            \
    libhash_hashfunc##BITS##_t hashfunc = NULL;                         \
                                                                        \
    if(narg <= 0 || narg > 3)                                           \
      luaL_error(L, "string, number, or tensor [string] [number] expected"); \
                                                                        \
    hashfunc = hashname2hashfunc##BITS(L, hashname);                        \
                                                                        \
    if(lua_isstring(L, 1)) {                                            \
      size_t len = 0;                                                   \
      const char *str = lua_tolstring(L, 1, &len);                      \
      hash = hashfunc(str, len, seed);                                  \
    }                                                                   \
    else if(lua_isnumber(L, 1)) {                                       \
      lua_Number num = lua_tonumber(L, 1);                              \
      hash = hashfunc(&num, sizeof(lua_Number), seed);                  \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.ByteTensor")) {                   \
      hash = THByteTensor_hash##BITS(luaT_toudata(L, 1, "torch.ByteTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.CharTensor")) {                   \
      hash = THCharTensor_hash##BITS(luaT_toudata(L, 1, "torch.CharTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.ShortTensor")) {                  \
      hash = THShortTensor_hash##BITS(luaT_toudata(L, 1, "torch.ShortTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.IntTensor")) {                    \
      hash = THIntTensor_hash##BITS(luaT_toudata(L, 1, "torch.IntTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.LongTensor")) {                   \
      hash = THLongTensor_hash##BITS(luaT_toudata(L, 1, "torch.LongTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.FloatTensor")) {                  \
      hash = THFloatTensor_hash##BITS(luaT_toudata(L, 1, "torch.FloatTensor"), hashfunc, seed); \
    }                                                                   \
    else if(luaT_isudata(L, 1, "torch.DoubleTensor")) {                 \
      hash = THDoubleTensor_hash##BITS(luaT_toudata(L, 1, "torch.DoubleTensor"), hashfunc, seed); \
    }                                                                   \
    else {                                                              \
      luaL_error(L, "string, number, or tensor [number] expected");     \
    }                                                                   \
    lua_pushnumber(L, (lua_Number)hash);                                \
    return 1;                                                           \
  }

IMPLEMENT_HASH(32);
IMPLEMENT_HASH(64);

int luaopen_libhash(lua_State *L)
{
  lua_getglobal(L, "require");
  if(!lua_isfunction(L, -1))
    luaL_error(L, "require seems not be a function");
  lua_pushstring(L, "torch");
  lua_call(L, 1, 1);
  if(!lua_istable(L, -1))
    luaL_error(L, "could not load torch");

  lua_pushstring(L, "hash");
  lua_pushcfunction(L, libhash_hash32);
  lua_rawset(L, -3);

  lua_pushstring(L, "hash64");
  lua_pushcfunction(L, libhash_hash64);
  lua_rawset(L, -3);

  return 1; /* torch */
}
