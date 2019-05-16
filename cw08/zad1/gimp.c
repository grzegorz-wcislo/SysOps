#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define clamp(V, X, Y) (((V) < (X)) ? (X) : (((V) > (Y)) ? (Y) : (V)))
#define ceil_div(X, Y) (((X) + (Y) - 1) / (Y))

int m;
static void *thread_block(void *params);
static void *thread_interleaved(void *params);

typedef struct flt {
    int size;
    double **buffer;
} flt;

typedef struct image {
    int width;
    int height;
    int **buffer;
} image;

image *input, *output;
flt *filter;

void calc_pixel(int, int);
void calc_column(int);

image *alloc_image(int, int);
image *empty_image(int, int);
image *open_image(char *);
void save_image(image*, char*);

flt *open_filter(char *filter_name);

int main(int argc, char* argv[])
{
    if (argc < 6)
        die("Pass:\n - number of threads\n - method\n - input\n - filter\n - output");

    m = parse_pos_int(argv[1]);
    char *method = argv[2];
    char *input_name = argv[3];
    char *filter_name = argv[4];
    char *output_name = argv[5];

    input = open_image(input_name);
    filter = open_filter(filter_name);
    output = empty_image(input->width, input->height);

    pthread_t *tids = malloc(m * sizeof(pthread_t));
    for (int k = 0; k < m; k++) {
        int *val = malloc(sizeof(int));
        *val = k;
        if (strcmp("block", method) == 0)
            pthread_create(tids + k, NULL, &thread_block, val);
        else
            pthread_create(tids + k, NULL, &thread_interleaved, val);
    }

    struct timeval start = curr_time();
    long total_time = 0;
    for (int k = 0; k < m; k++) {
        long *time;
        pthread_join(tids[k], (void *) &time);
        printf("thread %d:\t%ldus\n", k+1, *time);
        total_time += *time;
    }
    printf("total time:\t%ldus\n", time_diff(start, curr_time()));
    printf("sum time:\t%ldus\n", total_time);

    save_image(output, output_name);

    return 0;
}

/***** THREADS *****/

static void *thread_block(void *params)
{
    struct timeval start = curr_time();

    int k = (*(int*) params);

    int x0 = ceil_div(k * input->width, m);
    int x1 = ceil_div((k + 1) * input->width, m);
    for (int x = x0; x < x1; x++) {
        calc_column(x);
    }

    long *time = malloc(sizeof(long));
    *time = time_diff(start, curr_time());

    pthread_exit(time);
}

static void *thread_interleaved(void *params)
{
    struct timeval start = curr_time();

    int k = (*(int*) params);
    for (int x = k; x < input->width; x += m) {
        calc_column(x);
    }

    long *time = malloc(sizeof(long));
    *time = time_diff(start, curr_time());

    pthread_exit(time);
}

/***** COMPUTATIONS *****/

void calc_pixel(int x, int y)
{
    int c = filter->size;
    double pixel = 0;
    for (int i = 0; i < c; i++) {
        for (int j = 0; j < c; j++) {
            int y_ = clamp(y - ceil_div(c, 2) + i - 1, 0, input->height - 1);
            int x_ = clamp(x - ceil_div(c, 2) + j - 1, 0, input->width - 1);
            pixel += input->buffer[y_][x_] * filter->buffer[i][j];
        }
    }
    output->buffer[y][x] = round(pixel);
}

void calc_column(int x)
{
    for (int y = 0; y < input->height; y++)
        calc_pixel(x, y);
}

/***** IMAGE IO *****/

image *alloc_image(int width, int height)
{
    image *img = malloc(sizeof(image));
    if (img == NULL) die_errno();

    img->width = width;
    img->height = height;

    img->buffer = malloc(img->height * sizeof(int*));
    if (img-> buffer == NULL) die_errno();
    for (int i = 0; i < img->height; i++) {
        img->buffer[i] = malloc(img->width * sizeof(int));
        if (img->buffer[i] == NULL) die_errno();
    }
    return img;
}

image *empty_image(int width, int height)
{
    image *img = alloc_image(width, height);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            img->buffer[y][x] = 0;
        }
    }
    return img;
}

image *open_image(char *image_name)
{
    FILE *file = fopen(image_name, "r");
    if (file == NULL) die_errno();

    int width, height;

    fscanf(file, "P2 %i %i 255", &width, &height);

    image *img = alloc_image(width, height);

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            fscanf(file, "%i", &img->buffer[y][x]);
        }
    }

    fclose(file);

    return img;
}

void save_image(image *img, char* img_name)
{
    FILE *file = fopen(img_name, "w");
    if (file == NULL) die_errno();

    fprintf(file, "P2\n%i %i\n255\n", img->width, img->height);

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            fprintf(file, "%i", img->buffer[y][x]);
            if (x+1 != img->width) fputc(' ', file);
        }
        fputc('\n', file);
    }
}

/***** FILTER IO *****/

flt *open_filter(char *filter_name)
{
    FILE *file = fopen(filter_name, "r");
    if (file == NULL) die_errno();

    int size;
    fscanf(file, "%i", &size);

    flt *f = malloc(sizeof(flt));
    if (f == NULL) die_errno();

    f->size = size;

    f->buffer = malloc(f->size * sizeof(double*));
    if (f->buffer == NULL) die_errno();
    for (int i = 0; i < f->size; i++) {
        f->buffer[i] = malloc(f->size * sizeof(double));
        if (f->buffer[i] == NULL) die_errno();
    }

    for (int y = 0; y < f->size; y++) {
        for (int x = 0; x < f->size; x++) {
            fscanf(file, "%lf", &f->buffer[y][x]);
        }
    }

    fclose(file);

    return f;
}
