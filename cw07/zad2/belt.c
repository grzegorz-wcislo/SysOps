#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "belt.h"
#include "shared.h"

sem_t *semaphores[3];

char *home()
{
	char *home = getenv("HOME");
	if (home == NULL) die("$HOME is not set");
  return home;
}

key_t get_key()
{
    key_t key = ftok(home(), PROJ_ID);
    if (key == -1) die_errno();
    return key;
}

belt *get_belt(void)
{
    int shm_id = shm_open("/belt", O_RDWR, 0);
    if (shm_id == -1) die_errno();

    int size;
    if (read(shm_id, &size, sizeof(int)) == -1) die_errno();

    belt *b = mmap(NULL,
                   sizeof(belt) + size * sizeof(package),
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   shm_id,
                   0);
    if (b == (void*) -1) die_errno();

    close(shm_id);

    if ((semaphores[BUFF] = sem_open("/buff", O_RDWR)) == SEM_FAILED) die_errno();
    if ((semaphores[EMPTY] = sem_open("/empty", O_RDWR)) == SEM_FAILED) die_errno();
    if ((semaphores[FILL] = sem_open("/fill", O_RDWR)) == SEM_FAILED) die_errno();

    return b;
}

belt *create_belt(int size, int max_weight)
{
    int shm_id = shm_open("/belt", O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_id == -1) die_errno();

    if (ftruncate(shm_id, sizeof(belt) + size * sizeof(package)) == -1) die_errno();

    belt *b = mmap(NULL,
                   sizeof(belt) + size * sizeof(package),
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   shm_id,
                   0);

    if (b == (void*) -1) die_errno();

    close(shm_id);

    b->first = 0;
    b->last = 0;
    b->size = size;
    b->weight = 0;
    b->max_weight = max_weight;
    b->trucker_done = 0;
    b->emptied = 0;

    if ((semaphores[BUFF] = sem_open("/buff", O_RDWR | O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) die_errno();
    if ((semaphores[EMPTY] = sem_open("/empty", O_RDWR | O_CREAT | O_EXCL, 0666, b->size)) == SEM_FAILED) die_errno();
    if ((semaphores[FILL] = sem_open("/fill", O_RDWR | O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) die_errno();

    return b;
}

void delete_belt(belt *b)
{
    sem_unlink("/buff");
    sem_unlink("/empty");
    sem_unlink("/fill");
    shm_unlink("/belt");
}

void timestamp_package(package *pkg)
{
    gettimeofday(&pkg->loaded_at,NULL);
}

package create_package(int mass)
{
    package pkg = { mass, getpid(), {} };
    return pkg;
}

void put_package(belt *b, package pkg)
{
    int size, taken, weight, max_weight;
    struct timeval start_time = curr_time();

    int done = 0, first_try = 1, first_try_failed = 0;
    while (!done) {
        if (b->trucker_done) exit(0);
        if (first_try_failed == 2) {
            print_curr_time();
            printf(", PID:%d, waiting for conveyor belt to load a %dkg package\n", getpid(), pkg.mass);
        } else if (first_try_failed == 1) first_try_failed = 0;

        if (first_try) {
            first_try = 0;
            if (dec_sem_nonblock(b, EMPTY) == -1) {
                first_try_failed = 1;
                dec_sem(b, EMPTY);
            }
        } else {
            dec_sem(b, EMPTY);
        }
        dec_sem(b, BUFF);

        if (b->max_weight >= b->weight + pkg.mass && !b->trucker_done) {
            timestamp_package(&pkg);
            b->buffer[b->last] = pkg;
            b->last = (b->last + 1) % b->size;
            b->weight += pkg.mass;

            size = b->size;
            taken = (b->last + size - 1 - b->first) % size + 1;

            weight = b->weight;
            max_weight = b->max_weight;

            done = 1;
        }

        inc_sem(b, BUFF);
        if (done) {
            inc_sem(b, FILL);
        } else {
            inc_sem(b, EMPTY);
            if (first_try_failed == 1) first_try_failed = 2;
        }
    }

    print_time(pkg.loaded_at);
    printf(", PID:%d, %dkg package loaded after %ldus, conveyor belt: %d/%d %dkg/%dkg\n",
           pkg.loader,
           pkg.mass,
           time_diff(start_time, curr_time()),
           taken,
           size,
           weight,
           max_weight);
}

void get_package(belt *b, truck *t)
{
    int size, taken, weight, max_weight;

    package pkg;
    int done = 0, first_try = 1;
    while (!done) {
        if (first_try) {
            first_try = 0;
            if (dec_sem_nonblock(b, FILL) == -1) {
                if (b->trucker_done) exit(0);
                print_curr_time();
                printf(", waiting for packages to arrive\n");
                dec_sem(b, FILL);
            }
        } else {
            dec_sem(b, FILL);
        }
        dec_sem(b, BUFF);

        pkg = b->buffer[b->first];
        if (pkg.mass <= t->capacity) {
            b->first = (b->first + 1) % b->size;
            b->weight -= pkg.mass;
            t->capacity -= pkg.mass;

            size = b->size;
            taken = (b->last + size - b->first) % size;

            weight = b->weight;
            max_weight = b->max_weight;

            done = 1;
        }

        inc_sem(b, BUFF);
        if (done) {
            inc_sem(b, EMPTY);
        } else {
            inc_sem(b, FILL);
        }

        if (!done) {
            print_curr_time();
            printf(", unloading truck\n");
            print_curr_time();
            printf(", new truck arrived\n");
            t->capacity = t->max_capacity;
        }
    }

    print_curr_time();
    printf(", PID:%d, %dkg package loaded after %ldus, %dkg free, conveyor belt: %d/%d %dkg/%dkg\n",
           pkg.loader,
           pkg.mass,
           time_diff(pkg.loaded_at, curr_time()),
           t->capacity,
           taken,
           size,
           weight,
           max_weight);

    if (b->trucker_done && size == 0) exit(0);
}

void inc_sem(belt *b, int sem_num)
{
    while (sem_post(semaphores[sem_num]) == -1) {
        if (errno != EINTR || b->emptied) exit(0);
    }
}

void dec_sem(belt *b, int sem_num)
{
    while (sem_wait(semaphores[sem_num]) == -1) {
        if (errno != EINTR || b->emptied) exit(0);
    }
}

int dec_sem_nonblock(belt *b, int sem_num)
{
    if (sem_trywait(semaphores[sem_num]) == -1) {
        if (errno == EAGAIN || (errno == EINTR && !b->emptied)) return -1;
        else exit(0);
    }
    return 0;
}
