#define _DEFAULT_SOURCE (1)

#include <fmutex.h>

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <stdatomic.h>

// Allocates memory for and create
// Default state is unlocked
Fmutex *fmutex_create(void) {
    void *memory;
    Fmutex *mutex;

    memory = malloc(fmutex_advise());
    if (memory == NULL) {
        return NULL;
    }

    if(fmutex_init(&mutex, memory) != 0) {
        free(memory);
        memory = NULL;
        return NULL;
    }

    return mutex;
}

// Advise how much memory is needed
size_t fmutex_advise(void) {
    return 1 * sizeof(Fmutex);
}

// Advise for many
size_t fmutex_advisev(size_t num_mutexes) {
    return num_mutexes * (1 * sizeof(Fmutex));
}

// Initialize
// Default state is unlocked
int fmutex_init(Fmutex **dest, void *memory) {
    Fmutex *mutex;
    if(dest == NULL || memory == NULL) {
        return EINVAL;
    }

    if(memset(memory, 0, fmutex_advise()) != memory){
        return ENOTRECOVERABLE;
    }
    *dest = memory;
    mutex = *dest;
    // nothing should be dependent on mutex->locked at this point
    atomic_store_explicit(&(mutex->locked), false, memory_order_release);

    return 0;
}

// Initialize for many
int fmutex_initv(size_t num_mutexes, Fmutex *dest[], void *memory) {
    Fmutex *mutexes;
    if(num_mutexes == 0 || dest == NULL || memory == NULL) {
        return EINVAL;
    }

    if(memset(memory, 0, fmutex_advisev(num_mutexes)) != memory){
        return ENOTRECOVERABLE;
    }
    *dest = memory;
    mutexes = *dest;
    for(size_t i = 0; i < num_mutexes; i++) {
        atomic_store_explicit(&(mutexes[i].locked), false, memory_order_release);
    }

    return 0;
}

// Deinitialize
void fmutex_deinit(Fmutex *mutex) {
    if(mutex == NULL) {
        return;
    }

    // nothing should be dependent on mutex->locked when this is set
    atomic_store_explicit(&(mutex->locked), false, memory_order_release);

    return;
}

/*
 * Destroys a Fmutex that was allocated by fmutex_create.
 * Please, only use with memory allocated by fmutex_create!
 */
void fmutex_destroy(Fmutex *mutex) {
    if(mutex == NULL) {
        return;
    }

    fmutex_deinit(mutex);
    free(mutex);

    return;
}

// lock mutex
// forces calling thread to wait if currently locked
// can lead to deadlock
int fmutex_lock(Fmutex *mutex) {
    long res;
    uint32_t expected = false;

    if(mutex == NULL) {
        return EINVAL;
    }

    while(!(atomic_compare_exchange_strong_explicit(&(mutex->locked), &expected, true, memory_order_acq_rel, memory_order_acquire))) {
        // want to perform FUTEX_WAIT_PRIVATE syscall
        // Successful result i.e. result in which sem->counter != 0 can be signaled in two ways
        // First, syscall itself returns 0 if waited then awaken
        // Two, syscall failed with -1 as sem->counter was not 0, errno is EAGAIN

        errno = 0;
        res = syscall(SYS_futex, &(mutex->locked), FUTEX_WAIT_PRIVATE, true, NULL);
        if(res == 0 || errno == EAGAIN) {
            // proceed
        } else {
            printf("error with FUTEX_WAIT in %s = %d\n", __func__, errno);
            return errno;
        }
        errno = 0;
    }

    return 0;
}

// unlock mutex
// fails if mutex is already unlocked
int fmutex_unlock(Fmutex *mutex) {
    long res;
    uint32_t expected = true;
    bool success;
    if(mutex == NULL) {
        return EINVAL;
    }

    success = atomic_compare_exchange_strong_explicit(&(mutex->locked), &expected, false, memory_order_acq_rel, memory_order_acquire);
    if(success) {
        errno = 0;
        res = syscall(SYS_futex, &(mutex->locked), FUTEX_WAKE_PRIVATE, 1);
        if(res < 0) {
            printf("error with FUTEX_WAKE in %s = %d\n", __func__, errno);
            return res;
        }
    } else {
        // expected is update with the current value on failure
        assert(expected == false);
    }

    return 0;
}
