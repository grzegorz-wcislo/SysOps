#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include "utils.h"

// DEFS

struct cart {
    pthread_t tid;
    int free_spaces;
    sem_t free_spot;
    sem_t open_doors_exit;
};

struct coaster {
    int cart_count;
    int cart_capacity;
    int rides_count;
    int passenger_count;
    int parked_cart_i;
    sem_t free;
    sem_t exit;
    sem_t start;
    sem_t open_doors_enter;
    pthread_mutex_t cart_entrance;
    short running;
    struct cart *carts;
};

// FUN SIGNATURES

void stamp_message(char*, char*, int);

static void *cart(void*);
static void *passenger(void*);

// GLOBAL VARIABLES

struct coaster *roller_coaster;

pthread_t *passengers;

//FUNCTIONS

int main(int argc, char *argv[])
{
    // parsing
    int passenger_count, cart_count, cart_capacity, rides_count;
    if (argc != 5)
        die("Pass: passenger_count, cart_count, cart_capacity, rides_count");
    passenger_count = parse_pos_int(argv[1]);
    cart_count = parse_pos_int(argv[2]);
    cart_capacity = parse_pos_int(argv[3]);
    rides_count = parse_pos_int(argv[4]);

    if (passenger_count < cart_count * cart_capacity)
        die("Too few passengers");

    // roller coaster init
    roller_coaster = malloc(sizeof(struct coaster));
    if (roller_coaster == NULL) die_errno();

    roller_coaster->carts = malloc(cart_count * sizeof(struct cart));
    if (roller_coaster->carts == NULL) die_errno();
    roller_coaster->cart_count = cart_count;
    roller_coaster->rides_count = rides_count;
    roller_coaster->passenger_count= passenger_count;
    roller_coaster->parked_cart_i = 0;
    roller_coaster->cart_capacity = cart_capacity;
    roller_coaster->running = 1;

    for (int i = 0; i < roller_coaster->cart_count; i++) {
        roller_coaster->carts[i].free_spaces = cart_capacity;
    }

    // passengers init
    passengers = malloc(passenger_count * sizeof(pthread_t));

    // mutexes
    if (pthread_mutex_init(&roller_coaster->cart_entrance, NULL) != 0) die_errno();

    // semaphores
    if (sem_init(&roller_coaster->free, 0, 0) == -1) die_errno();
    if (sem_init(&roller_coaster->exit, 0, 0) == -1) die_errno();
    if (sem_init(&roller_coaster->start, 0, 0) == -1) die_errno();
    if (sem_init(&roller_coaster->open_doors_enter, 0, 0) == -1) die_errno();

    for (int i = 0; i < roller_coaster->cart_count; i++) {
        if (sem_init(&roller_coaster->carts[i].open_doors_exit, 0, 0) == -1) die_errno();
        if (sem_init(&roller_coaster->carts[i].free_spot, 0, 0) == -1) die_errno();
    }

    // unlock station semapthore
    sem_post(&roller_coaster->carts[0].free_spot);

    // starting processes
    for (int i = 0; i < roller_coaster->cart_count; i++) {
        int *index = malloc(sizeof(int));
        if (index == NULL) die_errno();
        *index = i;
        pthread_create(&roller_coaster->carts[i].tid, NULL, &cart, index);
    }

    for (int i = 0; i < passenger_count; i++) {
        int *index = malloc(sizeof(int));
        if (index == NULL) die_errno();
        *index = i;
        pthread_create(&passengers[i], NULL, &passenger, index);
    }

    // joining threads
    for (int i = 0; i < roller_coaster->cart_count; i++) {
        pthread_join(roller_coaster->carts[i].tid, NULL);
    }

    for (int i = 0; i < passenger_count; i++) {
        pthread_join(passengers[i], NULL);
    }

    return 0;
}


static void *cart(void *p)
{
    int index =  *((int*) p);
    struct cart *cart = &roller_coaster->carts[index];
    sem_t *free_spot = &cart->free_spot;

    char *msg_time = malloc(200);
    if (msg_time == NULL) die_errno();
    char *msg = msg_time + 50;

    for (int i = 0;; i++) {
        sem_wait(free_spot);

        // notify exiting
        pthread_mutex_lock(&roller_coaster->cart_entrance);
        stamp_message(msg, "cart", index);
        sprintf(msg + 30, "Opening doors");
        puts(msg);
        for (int j = 0; j < roller_coaster->cart_capacity; j++) {
            if (i > 0)
                sem_post(&cart->open_doors_exit);
            if (i < roller_coaster->rides_count)
                sem_post(&roller_coaster->open_doors_enter);
        }
        pthread_mutex_unlock(&roller_coaster->cart_entrance);

        // wait for exiting
        if (i > 0)
            for (int j = 0; j < roller_coaster->cart_capacity; j++)
                sem_wait(&roller_coaster->exit);

        // last ride
        if (i == roller_coaster->rides_count) {
            pthread_mutex_lock(&roller_coaster->cart_entrance);

            stamp_message(msg, "cart", index);
            sprintf(msg + 30, "Finishing work");
            puts(msg);

            // last cart
            if (index == roller_coaster->cart_count - 1) {
                roller_coaster->running = 0;
                for (int j = 0; j < roller_coaster->passenger_count; j++) {
                    sem_post(&roller_coaster->open_doors_enter);
                }
            } else {
                roller_coaster->parked_cart_i++;
                roller_coaster->parked_cart_i %= roller_coaster->cart_count;
                sem_post(&roller_coaster->carts[roller_coaster->parked_cart_i].free_spot);
            }
            pthread_mutex_unlock(&roller_coaster->cart_entrance);
            break;
        }

        // notify entering
        for (int j = 0; j < roller_coaster->cart_capacity; j++)
            sem_post(&roller_coaster->free);

        // wait for start
        sem_wait(&roller_coaster->start);

        pthread_mutex_lock(&roller_coaster->cart_entrance);
        stamp_message(msg, "cart", index);
        sprintf(msg + 30, "Closing doors");
        puts(msg);

        roller_coaster->parked_cart_i++;
        roller_coaster->parked_cart_i %= roller_coaster->cart_count;

        stamp_message(msg, "cart", index);
        sprintf(msg + 30, "Starting the ride");
        puts(msg);

        sem_post(&roller_coaster->carts[roller_coaster->parked_cart_i].free_spot);

        pthread_mutex_unlock(&roller_coaster->cart_entrance);

        // sleep

        usleep(rand() % 10000);

        pthread_mutex_lock(&roller_coaster->cart_entrance);
        stamp_message(msg, "cart", index);
        sprintf(msg + 30, "Finishing the ride");
        puts(msg);
        pthread_mutex_unlock(&roller_coaster->cart_entrance);
    }
    pthread_exit(0);
}

// PASSENGER

static void *passenger(void *p)
{
    int index =  *((int*) p);
    char *msg = malloc(200);
    if (msg == NULL) die_errno();

    while (1) {
        // enter cart
        sem_wait(&roller_coaster->open_doors_enter);
        if (!roller_coaster->running) {
            // stop if the last cart is stopped
            pthread_mutex_lock(&roller_coaster->cart_entrance);
            stamp_message(msg, "pass", index);
            sprintf(msg + 30, "Finishing fun");
            puts(msg);
            pthread_mutex_unlock(&roller_coaster->cart_entrance);
            break;
        }
        sem_wait(&roller_coaster->free);

        pthread_mutex_lock(&roller_coaster->cart_entrance);
        struct cart *current_cart = &roller_coaster->carts[roller_coaster->parked_cart_i];

        current_cart->free_spaces--;

        stamp_message(msg, "pass", index);
        sprintf(msg + 30, "Entered the cart: %d passengers inside", roller_coaster->cart_capacity - current_cart->free_spaces);
        puts(msg);

        // start cart
        if (current_cart->free_spaces == 0) {
            stamp_message(msg, "pass", index);
            sprintf(msg + 30, "Pushing start");
            puts(msg);
            sem_post(&roller_coaster->start);
        }
        pthread_mutex_unlock(&roller_coaster->cart_entrance);

        // wait for open doors
        sem_wait(&current_cart->open_doors_exit);

        // exit cart
        pthread_mutex_lock(&roller_coaster->cart_entrance);
        sem_post(&roller_coaster->exit);
        current_cart->free_spaces++;

        stamp_message(msg, "pass", index);
        sprintf(msg + 30, "Exited the cart: %d passengers inside", roller_coaster-> cart_capacity - current_cart->free_spaces);
        puts(msg);
        pthread_mutex_unlock(&roller_coaster->cart_entrance);

        sched_yield();
    }
    pthread_exit(0);
}

void stamp_message(char* msg, char* type, int id)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    snprintf(msg, 31,  "%ld%ld: %4s%d:                    ", t.tv_sec, t.tv_usec, type, id+1);
}
