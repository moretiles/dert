#define _DEFAULT_SOURCE (1)

#include <fmutex.h>
#include <fsemaphore.h>
#include <pointerarith.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

Fsemaphore *fsemaphore_create(uint64_t val, uint64_t max) {
    Fsemaphore *sem;
    void *memory;
    if(max == 0) {
        return NULL;
    }

    memory = malloc(fsemaphore_advise(val, max));
    if(memory == NULL) {
        return NULL;
    }

    if(fsemaphore_init(&sem, memory, val, max) != 0) {
        free(memory);
        memory = NULL;
        return NULL;
    }

    return sem;
}

size_t fsemaphore_advise(uint64_t val, uint64_t max) {
    // eliminate unused warnings
    val = val;
    max = max;

    return 1 * ((1 * sizeof(Fsemaphore)) +
                (fmutex_advise()));
}

size_t fsemaphore_advisev(size_t num_sems, uint64_t val, uint64_t max) {
    // eliminate unused warnings
    val = val;
    max = max;

    return num_sems * ((1 * sizeof(Fsemaphore)) +
                       (fmutex_advise()));
}

int fsemaphore_init(Fsemaphore **dest, void *memory, uint64_t val, uint64_t max) {
    Fsemaphore *sem;
    if(dest == NULL || memory == NULL || max == 0) {
        return EINVAL;
    }

    *dest = memory;
    sem = memory;
    memory = pointer_literal_addition(memory, sizeof(Fsemaphore));
    if(fmutex_init(&(sem->mutex), memory) != 0) {
        return EINVAL;
    }

    sem->counter = val;
    sem->max = max;

    return 0;
}

int fsemaphore_initv(size_t num_sems, Fsemaphore *dest[], void *memory, uint64_t val, uint64_t max) {
    Fsemaphore **sems;
    if(dest == NULL || memory == NULL || max == 0) {
        return EINVAL;
    }

    *dest = memory;
    sems = memory;
    for(uint64_t i = 0; i < num_sems; i++) {
        memory = pointer_literal_addition(memory, sizeof(Fsemaphore));
        if(fmutex_init(&(sems[i]->mutex), memory) != 0) {
            return EINVAL;
        }
        memory = pointer_literal_addition(memory, fmutex_advise());

        sems[i]->counter = val;
        sems[i]->max = max;
    }

    return 0;
}

void fsemaphore_deinit(Fsemaphore *sem) {
    if(sem == NULL) {
        return;
    }

    fmutex_deinit(sem->mutex);
    memset(sem, 0, sizeof(Fsemaphore));

    return;
}

void fsemaphore_destroy(Fsemaphore *sem) {
    if(sem == NULL) {
        return;
    }

    fsemaphore_deinit(sem);
    free(sem);

    return;
}

int fsemaphore_exhaust(Fsemaphore *sem) {
    int res;
    if(sem == NULL) {
        return EINVAL;
    }

    res = fmutex_lock(sem->mutex);
    if(res != 0) {
        return res;
    }
    sem->counter = 0;
    res = fmutex_unlock(sem->mutex);
    if(res != 0) {
        return res;
    }

    return 0;
}

int fsemaphore_wait(Fsemaphore *sem) {
    long res;
    bool mutex_locked = false;
    bool decremented = false;
    if(sem == NULL) {
        return EINVAL;
    }

    while(!decremented) {
        res = fmutex_lock(sem->mutex);
        if(res != 0) {
            goto fsemaphore_wait_error;
        }
        mutex_locked = true;
        if(sem->counter > 0) {
            sem->counter -= 1;
            decremented = true;
        } else {
            res = fmutex_unlock(sem->mutex);
            if(res != 0) {
                goto fsemaphore_wait_error;
            }
            mutex_locked = false;
            printf("sleeping\n");

            // want to perform FUTEX_WAIT_PRIVATE syscall
            // Successful result i.e. result in which sem->counter != 0 can be signaled in two ways
            // First, syscall itself returns 0 if waited then awaken
            // Two, syscall failed with -1 as sem->counter was not 0, errno is EAGAIN

            errno = 0;
            res = syscall(SYS_futex, &(sem->counter), FUTEX_WAIT_PRIVATE, 0, NULL);
            if(res == 0 || errno == EAGAIN) {
                // proceed
            } else {
                printf("error with FUTEX_WAIT in %s = %d\n", __func__, errno);
                res = errno;
                goto fsemaphore_wait_error;
            }
            errno = 0;

            printf("awaken\n");
        }
    }

fsemaphore_wait_error:
    if(mutex_locked) {
        fmutex_unlock(sem->mutex);
        mutex_locked = false;
    }
    return res;
}

int fsemaphore_post(Fsemaphore *sem) {
    bool mutex_locked = false;
    long res;

    res = fmutex_lock(sem->mutex);
    if(res != 0) {
        goto fsemaphore_post_error;
    }
    mutex_locked = true;
    if(sem->counter == sem->max) {
        res = EDEADLK;
        goto fsemaphore_post_error;
    } else {
        sem->counter += 1;
        res = fmutex_unlock(sem->mutex);
        if(res != 0) {
            goto fsemaphore_post_error;
        }
        mutex_locked = false;

        errno = 0;
        res = syscall(SYS_futex, &(sem->counter), FUTEX_WAKE_PRIVATE, 1);
        if(res < 0) {
            printf("error with FUTEX_WAKE in %s = %d\n", __func__, errno);
            goto fsemaphore_post_error;
        }
    }

fsemaphore_post_error:
    if(mutex_locked) {
        fmutex_unlock(sem->mutex);
        mutex_locked = false;
    }
    return 0;
}

int fsemaphore_reset(Fsemaphore *sem) {
    int res;
    if(sem == NULL) {
        return EINVAL;
    }

    res = fmutex_lock(sem->mutex);
    if(res != 0) {
        return res;
    }
    sem->counter = sem->max;
    res = fmutex_unlock(sem->mutex);
    if(res != 0) {
        return res;
    }

    return 0;
}
