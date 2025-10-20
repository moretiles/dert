# Do Everything RighT
* A collection of data structures that align with what I think is reasonable.
* This is a personal library so there may be breaking changes to APIs at **any** time.
* Supports C11 or later

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
* Remove mutex from pool. In this case, syncronization of resources should be left to user.
* Consider removing functions attached to pool.
* Attach function pointer to vht for producing "key" when evaluating the hash of a given key.
* Standardize public API naming.
* Standardize returned errors.
