# Do Everything RighT
* A collection of data structures that align with what I think is reasonable.
* This is a personal library so there may be breaking changes to APIs at **any** time.
* Supports C11 or later

## General purpose allocators
* Arenas.
* Pools.

## Dynamic length
* Dynamic Length Array.
* Doubly linked list.
* Hashmap -- built using two parallel varray.

## Fixed length
* Twin Buffers.
* Stack.
* Queue.
* Ring Buffer (Queue data structure with overwrite enabled).
* File Buffered Queue.
* Atomic single-producer, single-consumer queue.
* Multiple-producer, single-consumer queue.

## Synchronization
* Thread pool.
* Futex-Backed Mutex (Linux only).
* Futex-Backed Semaphore (Linux only).

## Random
* Good version of cstrncpy.
* Short, portable names for fixed width signed integers, unsigned integers, and reals.
* Function for literal addition on top of pointer.

# How to use
```sh
# prepare required dependencies
git submodule update --init --recursive SipHash/

# install optional dependencies and run tests (optional)
# to do this you need to have installed a version of graphviz built with cgraph
make test
./test

# build library
make all
```

# TODO:
* Look into why gcc thread sanitizer fails to build when compiling with .S files that use global labels.
* Standardize public API naming.
* Standardize returned errors.
