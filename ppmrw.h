/* ppmrw header file */
#ifndef PPMRW_H
#define PPMRW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE 0
#define TRUE 1
#define MAX_SIZE 1024

/* variables and types */
typedef int8_t boolean;

// file header info
typedef struct header_t {
    int file_type;
    char **comments;
    int width;
    int height;
    int max_color_val;
} header;

// one pixel
typedef struct RGBPixel_t {
    unsigned char r, g, b;
} RGBPixel;

// image info
typedef struct image_t {
    RGBPixel *pixmap;
    int width, height, max_color_val;
} image;

int read_header(FILE *fh, header *hdr);
int read_p6_data(FILE *fh, image *img);
int read_p3_data(FILE *fh, image *img);

#endif
