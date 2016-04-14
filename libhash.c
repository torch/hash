#include <lua.h>
#include <lauxlib.h>

#include "luaT.h"
#include "TH.h"
#include "hash.h"

#define LH_MAX_MOD 9007199254740992L

#define IMPLEMENT_THTENSOR_HASH(TYPE, CTYPE)                            \
  static void TH##TYPE##Tensor_hashUpdate(TH##TYPE##Tensor *tensor, LHHash *hash) \
  {                                                                     \
    TH_TENSOR_APPLY(CTYPE, tensor,                                      \
                    LHHash_update(hash, tensor_data, tensor_size*sizeof(CTYPE)); break;); \
  }

IMPLEMENT_THTENSOR_HASH(Byte, unsigned char);
IMPLEMENT_THTENSOR_HASH(Char, char);
IMPLEMENT_THTENSOR_HASH(Short, short);
IMPLEMENT_THTENSOR_HASH(Int, int);
IMPLEMENT_THTENSOR_HASH(Long, long);
IMPLEMENT_THTENSOR_HASH(Float, float);
IMPLEMENT_THTENSOR_HASH(Double, double);

static void libhash_updatehash(lua_State *L, LHHash *state, int idx)
{
  if(lua_type(L, idx) == LUA_TSTRING) {
    size_t len = 0;
    const char *str = lua_tolstring(L, idx, &len);
    LHHash_update(state, str, len);
  }
  else if(lua_type(L, idx) == LUA_TNUMBER) {
    lua_Number num = lua_tonumber(L, idx);
    LHHash_update(state, &num, sizeof(lua_Number));
  }
  else if(luaT_isudata(L, idx, "torch.ByteTensor")) {
    THByteTensor_hashUpdate(luaT_toudata(L, idx, "torch.ByteTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.CharTensor")) {
    THCharTensor_hashUpdate(luaT_toudata(L, idx, "torch.CharTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.ShortTensor")) {
    THShortTensor_hashUpdate(luaT_toudata(L, idx, "torch.ShortTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.IntTensor")) {
    THIntTensor_hashUpdate(luaT_toudata(L, idx, "torch.IntTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.LongTensor")) {
    THLongTensor_hashUpdate(luaT_toudata(L, idx, "torch.LongTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.FloatTensor")) {
    THFloatTensor_hashUpdate(luaT_toudata(L, idx, "torch.FloatTensor"), state);
  }
  else if(luaT_isudata(L, idx, "torch.DoubleTensor")) {
    THDoubleTensor_hashUpdate(luaT_toudata(L, idx, "torch.DoubleTensor"), state);
  }
  else {
    luaL_error(L, "string, number, or tensor [number] expected");
  }
}

/*
  stuff [seed] [mod]
  stuff name [seed] [mod]
  stuff hash [seed] [mod]
 */
static int libhash_hash(lua_State *L)
{
  int nopt = lua_gettop(L);
  LHHash *state = NULL;
  unsigned long long seed = 0;
  int freestate = 0;
  unsigned long long mod = 0;
  unsigned long long res = 0;

  if(nopt == 1 || (nopt >= 2 && nopt <= 3 && lua_isnumber(L, 2))) {
    state = LHXXH64_new();
    seed = (unsigned long long)luaL_optlong(L, 2, 0);
    mod = (unsigned long long)luaL_optlong(L, 3, LH_MAX_MOD);
    freestate = 1;
  } else if((nopt >= 2 && nopt <= 4) && lua_tostring(L, 2)) {
    const char *hashtype = lua_tostring(L, 2);
    seed = (unsigned long long)luaL_optlong(L, 3, 0);
    mod = (unsigned long long)luaL_optlong(L, 4, LH_MAX_MOD);
    if(!strcmp(hashtype, "XXH64"))
      state = LHXXH64_new();
    else if(!strcmp(hashtype, "FNV64"))
      state = LHFNV64_new();
    else
      luaL_error(L, "invalid hash type (XXH64 || FNV64 expected)");
    freestate = 1;
  } else if((nopt >= 2 && nopt <= 4) && luaT_toudata(L, 2, "torch.Hash")) {
    state = luaT_toudata(L, 2, "torch.Hash");
    seed = (unsigned long long)luaL_optlong(L, 3, 0);
    mod = (unsigned long long)luaL_optlong(L, 4, LH_MAX_MOD);
  } else {
    luaL_error(L, "invalid arguments: stuff [seed] [mod] || stuff name [seed] [mod] || stuff hash [seed] [mod] expected");
  }

  if(!state)
    luaL_error(L, "invalid Hash state");
  LHHash_reset(state, seed);
  libhash_updatehash(L, state, 1);
  res = LHHash_digest(state);
  res = res % mod;
  if(freestate)
    LHHash_free(state);

  lua_pushnumber(L, res);
  return 1;
}

static int libhash_LHXXH64_new(lua_State *L)
{
  LHHash *state = LHXXH64_new();
  unsigned long long seed = (unsigned long long)luaL_optlong(L, 1, 0);
  LHHash_reset(state, seed);
  luaT_pushudata(L, state, "torch.Hash");
  return 1;
}

static int libhash_LHFNV64_new(lua_State *L)
{
  LHHash *state = LHFNV64_new();
  unsigned long long seed = (unsigned long long)luaL_optlong(L, 1, 0);
  LHHash_reset(state, seed);
  luaT_pushudata(L, state, "torch.Hash");
  return 1;
}

static int libhash_LHHash_reset(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  unsigned long long seed = (unsigned long long)luaL_optlong(L, 2, 0);
  LHHash_reset(state, seed);
  return 1; /* self */
}

static int libhash_LHHash_update(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  libhash_updatehash(L, state, 2);
  return 1; /* self */
}

static int libhash_LHHash_hash(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  unsigned long long seed = (unsigned long long)luaL_optlong(L, 3, 0);
  unsigned long long mod = (unsigned long long)luaL_optlong(L, 4, LH_MAX_MOD);
  unsigned long long res = 0;
  luaL_argcheck(L, mod > 0, 4, "modulo should be positive");
  LHHash_reset(state, seed);
  libhash_updatehash(L, state, 2);
  res = LHHash_digest(state);
  res %= mod;
  lua_pushnumber(L, res);
  return 1; /* self */
}

static int libhash_LHHash_digest(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  unsigned long long mod = (unsigned long long)luaL_optlong(L, 2, LH_MAX_MOD);
  luaL_argcheck(L, mod > 0, 2, "modulo should be positive");
  unsigned long long res = LHHash_digest(state) % mod;
  lua_pushinteger(L, (long)res);
  return 1;
}

static int libhash_LHHash_clone(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  LHHash *newstate = LHHash_clone(state);
  luaT_pushudata(L, newstate, "torch.Hash");
  return 1;
}

static int libhash_LHHash_free(lua_State *L)
{
  LHHash *state = luaT_checkudata(L, 1, "torch.Hash");
  LHHash_free(state);
  return 0;
}

static const struct luaL_Reg libhash_LHHash__ [] = {
  {"hash", libhash_LHHash_hash},
  {"reset", libhash_LHHash_reset},
  {"update", libhash_LHHash_update},
  {"digest", libhash_LHHash_digest},
  {"clone", libhash_LHHash_clone},
  {NULL, NULL}
};

static const struct luaL_Reg libhash__ [] = {
  {"XXH64", libhash_LHXXH64_new},
  {"FNV64", libhash_LHFNV64_new},
  {"hash", libhash_hash},
  {NULL, NULL}
};

int luaopen_libhash(lua_State *L)
{
  lua_getglobal(L, "require");
  if(!lua_isfunction(L, -1))
    luaL_error(L, "require seems not be a function");
  lua_pushstring(L, "torch");
  lua_call(L, 1, 1);
  if(!lua_istable(L, -1))
    luaL_error(L, "could not load torch");

  luaT_newmetatable(L, "torch.Hash", NULL, NULL, libhash_LHHash_free, NULL);
  luaL_register(L, NULL, libhash_LHHash__);
  lua_pop(L, 1);

  lua_newtable(L);
  luaL_register(L, NULL, libhash__);

  return 1; /* hash */
}
