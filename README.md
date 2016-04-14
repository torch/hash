Hash functions for Torch
========================

This package provides few hashing capabilities for Torch. At this time it supports both XXH64 and FNV64 hashes. By default, XXH64 hash is used (much faster on large chunk of data).

Data which can be hashed is Lua strings, Lua numbers, or CPU Torch tensor types (Byte, Char, Short, Int, Long, Float, Double).

Concerning the computation of the tensor hash, only the data (not the shape) of the tensor is considered. Two tensors containing the same data, but with different strides, will thus have the exact same hash. However, computing the hash of a non-contiguous tensor is much slower than computing the hash of a contiguous tensor.

# Usage

We assume in the following that the package is loaded:
```lua
local hash = require 'hash'
```

There are two ways of using the package. Either through
```lua
hash.hash(...)
```
or by explicitely creating a hash state:
```lua
local state = hash.XXH64()
state:hash(...)
```

The second way is faster if one has to hash a lot of different things (as it avoids re-creating memory for the state at each time).
There are also several refined methods which are available for the hash states.

# Functions creating implicitely a state

## hash.hash(stuff, [seed], [mod])

Returns a 64 bits hash, modulo `mod`. The hash algorithm is XXH64. A seed can be provided if needed (0 by default). Mod is `2^53` by default,
which is the largest long value that a double can store (note that Lua numbers are doubles).

`stuff` might be either a Lua string, a Lua number, or a CPU tensor type (Byte, Char, Short, Int, Long, Float, Double).

## hash.hash(stuff, hashname, [seed], [mod])

Returns a 64 bits hash, modulo `mod`. The hash algorithm is given by `hashname` and can be the string `XXH64` or `FNV64`. A seed can be provided if needed (0 by default). Mod is `2^53` by default,
which is the largest long value that a double can store (note that Lua numbers are doubles).

`stuff` might be either a Lua string, a Lua number, or a CPU tensor type (Byte, Char, Short, Int, Long, Float, Double).

## hash.hash(stuff, hash, [seed], [mod])

Returns a 64 bits hash, modulo `mod`. A previously created hash `state` is given (see below for how to create it). A seed can be provided if needed (0 by default). Mod is `2^53` by default,
which is the largest long value that a double can store (note that Lua numbers are doubles).

`stuff` might be either a Lua string, a Lua number, or a CPU Torch tensor type (Byte, Char, Short, Int, Long, Float, Double).

# Functions creating explicitely a state

## hash.XXH64([seed])

Returns a new XXH64 hash state. By default `seed` is 0.

## hash.FNV64([seed])

Returns a new FNV64 hash state. By default `seed` is 0.

## Methods for hash states

### state:reset([seed])

Reset the given state using the given seed.

### state:update(stuff)

Hash given `stuff` and update the state accordingly. `stuff` might be a Lua string, a Lua number, or a CPU Torch tensor.
This method can be called several times in a row, if needed.

### state:digest([mod])

Returns the current hash (modulo `mod`) for the data which has been given to the state so far (with `update()`). By default `mod` is `2^53`, which
is the largest long value that a double can store with no precision loss.

Consecutive calls of `digest()` will return the same hash.

### state:hash(stuff, [seed], [mod])

Hash `stuff`, by first calling `reset()` with the given `seed` (by default `seed` is 0). Returns (with a call to `digest()`)
the hash, modulo `mod`. By default `mod` is `2^53`.

`stuff` might be either a Lua string, a Lua number, or a CPU Torch tensor type (Byte, Char, Short, Int, Long, Float, Double).

### state:clone()

Returns a new state which is a clone of the given one.
