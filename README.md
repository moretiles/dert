# Some useful C files
**Supports C11 or later**

## General purpose allocators
* Arenas.
* Pools.

## General purpose data structures that function for all types
* Dynamic Length Array.
* Stack.
* Queue.
* Ring Buffer (Queue data structure with overwrite enabled).
* Doubly linked list.
* Hashmap -- built using two parallel varray.

## FILE I/O
* File Buffered Queue

## Random
* Good version of cstrncpy.
* Short, portable names for fixed width signed integers, unsigned integers, and reals.
* Assert macro to make debugging with assert statements easier for Clang and MSVC.
* Function for literal addition on top of pointer.

# How to use
```sh
# prepare dependencies
git submodule update --init --recursive

# make and run tests (optional)
make test
./test

# build library
make
```

# TODO:
* Remove mutex from pool. In this case syncronization of resources should be left to user.
* Consider removing functions attached to pool.
* Attach function to vht for selecting "key" from the pointed to key.
* Standardize public API naming.
* Standardize returned errors.
