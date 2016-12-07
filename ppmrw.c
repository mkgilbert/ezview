/** ppmrw program for reading and writing images in ppm format
 * Author: Michael Gilbert
 * CS430 - Computer Graphics
 * Project 1
 * usage: ppmrw 3|6 <infile> <outfile>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "ppmrw.h"


/*******************************************************//**
 * Utility functions
 * ********************************************************/

/**
 * Checks for and moves over consecutive comments in a file recursively
 * @param fh file stream being checked
 * @param c The last character that was read from the file.
 * @return 0 on success, -1 on error
 */
int check_for_comments(FILE *fh, char c) {
    /* jumps over comments and onto the next line, then recursively checks again */
    // skip any leading white space
    while (isspace(c) && c != EOF) { c = fgetc(fh); }

    // base case, current char, c, is not a pound sign
    if (c != '#') {
        fseek(fh, -1, SEEK_CUR); // backup one character
        return 0;
    }
    else { // c is a comment, so read to end of line
        while (c != '\n' && c != EOF) {
            c = fgetc(fh);
        }
        if (c == EOF) {
            fprintf(stderr, "Error: check_for_comments: Premature end of file\n");
            return -1;
        }
        else { // c is '\n', so grab the next char and check recursively
            return check_for_comments(fh, fgetc(fh));
        }
    }
}

int check_for_newline(char c) {
    if (!isspace(c)) {
        fprintf(stderr, "Error: check_for_newline: missing newline or space\n");
        return -1;
    }
    return 0;
}

int bytes_left(FILE *fh) {
    // returns the number of bytes left in a file
    int bytes;
    int pos = ftell(fh);    // get current pointer
    fseek(fh, 0, SEEK_END);
    int end = ftell(fh);
    bytes = end - pos;
    fseek(fh, pos, SEEK_SET); // put the pointer back
    if (bytes <= 0) {
        fprintf(stderr, "Error: bytes_left: bytes remaining <= 0\n");
        return -1;
    }
    return bytes;
}


/*******************************************************//**
 * PPM read/write functions
 * ********************************************************/

/**
 * Reads the header information from a ppm file from a file stream into
 * a header struct
 * @param fh file handler
 * @param hdr header struct to store the information from the fh stream
 * @return 0 on success, -1 on error
 */
int read_header(FILE *fh, header *hdr) {
    int ret_val;    // holds temp return value for reading each section of header
    char c;         // temporary char read in from file
    boolean is_p3;  // determines file type being P3 or P6

    // read magic number
    c = fgetc(fh);
    if (c != 'P') {
        fprintf(stderr, "Error: read_header: Invalid ppm file. First character is not 'P'\n");
        return -1;
    }
    c = fgetc(fh);
    if (c == '3') {
        is_p3 = TRUE;
    }
    else if (c == '6') {
        is_p3 = FALSE;
    }
    else {
        fprintf(stderr, "Error: read_header: Unsupported magic number found in header\n");
        return -1;
    }
    // set header struct magic number
    if (is_p3) {
        hdr->file_type = 3;
    }
    else {
        hdr->file_type = 6;
    }
    // check for newline and comments
    ret_val = check_for_newline(fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: No separator found after magic number\n");
        return -1;
    }
    ret_val = check_for_comments(fh, fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: Problem reading comment after magic number\n");
        return -1;
    }

    // read width
    fscanf(fh, "%d", &(hdr->width));
    if (hdr->width <= 0) {
        fprintf(stderr, "Error: read_header: Image width cannot be less than zero\n");
        return -1;
    }
    if (hdr->width == EOF) {
        fprintf(stderr, "Error: read_header: Image width not found. Premature EOF\n");
        return -1;
    }
    // check for newline and comments
    ret_val = check_for_newline(fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: No separator found after width\n");
        return -1;
    }
    ret_val = check_for_comments(fh, fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: Problem reading comment after width\n");
        return -1;
    }

    // read height
    ret_val = fscanf(fh, "%d", &(hdr->height));
    if (ret_val <= 0 || ret_val == EOF) {
        fprintf(stderr, "Error: read_header: Image height not found\n");
        return -1;
    }
    // check for newline and comments
    ret_val = check_for_newline(fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: No separator found after height\n");
        return -1;
    }
    ret_val = check_for_comments(fh, fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: Problem reading comment after height\n");
        return -1;
    }
    
    // read max color value
    ret_val = fscanf(fh, "%d", &(hdr->max_color_val));
    if (ret_val <= 0 || ret_val == EOF) {
        fprintf(stderr, "Error: read_header: Max color value not found\n");
        return -1;
    }
    // check bounds on max color value
    if (hdr->max_color_val > 255 || hdr->max_color_val < 0) {
        fprintf(stderr, "Error: max color value must be >= 0 and <= 255\n");
        return -1;
    }
    // check for newline and comments for the last time
    ret_val = check_for_newline(fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: No separator found after max color value\n");
        return -1;
    }
    ret_val = check_for_comments(fh, fgetc(fh));
    if (ret_val < 0) {
        fprintf(stderr, "Error: read_header: Problem reading comment after max color value\n");
        return -1;
    }

    return 0;
}

/**
 * Writes ppm P6 image data (pixels) to a file stream
 * @param fh file handler
 * @param img image struct holding image data to be written
 * @return 0 on success, -1 on error
 */
int write_p6_data(FILE *fh, image *img) {
    int i,j;
    for (i=0; i<(img->height); i++) {
        for (j=0; j<(img->width); j++) {
            fwrite(&(img->pixmap[i * img->width + j].r), 1, 1, fh);
            fwrite(&(img->pixmap[i * img->width + j].g), 1, 1, fh);
            fwrite(&(img->pixmap[i * img->width + j].b), 1, 1, fh);
        }
    }
    return 0;
}

/**
 * Reads the pixel data from a P6 ppm file from a file stream into
 * an img struct
 * @param fh input file pointer
 * @param img initially empty. Place to store image data read from fh
 * @return 0 on success, -1 on error
 */
int read_p6_data(FILE *fh, image *img) {
    // reads p6 data and stores in buffer
    // read all remaining data from image into buffer
    int b = bytes_left(fh);
    // check for error reading bytes_left()
    if (b < 0) {
        fprintf(stderr, "Error: read_p6_data: Problem reading remaining bytes in image\n");
        return -1;
    }

    // create temp buffer for image data + 1 for null char
    unsigned char data[b+1];
    int read;

    // read the rest of the file and check that what remains is the right size
    if ((read = fread(data, 1, b, fh)) < 0) {
        fprintf(stderr, "Error: read_p6_data: fread() returned an error when reading data\n");
        return -1;
    }

    // double check number of bytes actually read is correct
    if (read < b || read > b) {
        fprintf(stderr, "Error: read_p6_data: image data doesn't match header dimensions\n");
        return -1;
    }

    int ptr = 0;        // data pointer/incrementer
    int i, j, k;        // loop variables
    unsigned char num;  // build a number from chars read in from file

    // loop through buffer and populate RGBPixel array
    for (i=0; i<img->height; i++) {
        for (j=0; j<img->width; j++) {
            RGBPixel px;
            for (k=0; k<3; k++) {
                // check that we haven't read more than what is available
                if (ptr >= b) {
                    fprintf(stderr, "Error: read_p6_data: Image data is missing or header dimensions are wrong\n");
                    return -1;
                }
                num = data[ptr++];
                if (num < 0 || num > img->max_color_val) {
                    fprintf(stderr, "Error: read_p6_data: found a pixel value out of range\n");
                    return -1;
                }

                if (k == 0) {
                    px.r = num;
                }
                else if (k == 1) {
                    px.g = num;
                }
                else {
                    px.b = num;
                }
            }
            img->pixmap[i * img->width + j] = px;
        }
    }
    // check if there's still data left
    if (ptr < b) {
        fprintf(stderr, "Error: read_p6_data: Extra image data was found in file\n");
        return -1;
    }
    return 0;
}

/**
 * Reads the pixel data from a P3 ppm file from a file stream into
 * an img struct
 * @param fh input file pointer
 * @param img initially empty. Place to store image data read from fh
 * @return 0 on success, -1 on error
 */
int read_p3_data(FILE *fh, image *img) {
    
    // read all remaining data from image into buffer
    int b = bytes_left(fh); 
    // check for error reading bytes_left()
    if (b < 0) {
        fprintf(stderr, "Error: read_p3_data: reading remaining bytes\n");
        return -1;
    }

    // create temp buffer for image data + 1 for null char
    char data[b+1];
    char *data_p = data;
    int read;

    // read the rest of the file and check that what remains is the right size
    if ((read = fread(data, 1, b, fh)) < 0) {
        fprintf(stderr, "Error: read_p3_data: fread returned an error when reading data\n");
        return -1;
    }
    
    // double check number of bytes actually read is correct
    if (read < b || read > b) {
        fprintf(stderr, "Error: read_p3_data: image data doesn't match header dimensions\n");
        return -1;
    }
    // null terminate the buffer
    data[b] = '\0';
    // make sure we're not starting at a space or newline
    while (isspace(*data_p) && (*data_p != '\0')) { data_p++; };
    
    int i, j, k;        // loop variables
    int ptr;            // current index of the num array
    char num[4];        // holds string repr. of a 0-255 value

    // loop through buffer and populate RGBPixel array
    for (i=0; i<img->height; i++) {
        for (j=0; j<img->width; j++) {
            RGBPixel px;
            for (k=0; k<3; k++) {
                ptr = 0;
                while (TRUE) {
                    // check that we haven't read more than what is available
                    if (*data_p == '\0') {
                        fprintf(stderr, "Error: read_p3_data: Image data is missing or header dimensions are wrong\n");
                        return -1;
                    }
                    if (isspace(*data_p)) {
                        *(num + ptr) = '\0';
                        while (isspace(*data_p) && (*data_p != '\0')) { 
                            data_p++; 
                        }
                        break;
                    }
                    else {
                        *(num + ptr) = *data_p++;
                        ptr++;
                    }
                }

                if (atoi(num) < 0 || atoi(num) > img->max_color_val) {
                    fprintf(stderr, "Error: read_p3_data: found a pixel value out of range\n");
                    return -1;
                }

                if (k == 0) {
                    px.r = atoi(num);
                }
                else if (k == 1) {
                    px.g = atoi(num);
                }
                else {
                    px.b = atoi(num);
                }
                img->pixmap[i * img->width + j] = px;
            }
        }
    }

    // skip any white space that may remain at the end of the data
    while (isspace(*data_p) && (*data_p != '\0')) { data_p++; };
    
    // check if there's still data left
    if (*data_p != '\0') {
        fprintf(stderr, "Error: read_p3_data: Extra image data was found in file\n");
        return -1;
    }
    return 0;
}

/**
 * Writes ppm P3 image data (pixels) to a file stream
 * @param fh file handler
 * @param img image struct holding image data to be written
 * @return 0 on success, -1 on error
 */
int write_p3_data(FILE *fh, image *img) {
    int i,j;
    for (i=0; i<(img->height); i++) {
        for (j=0; j<(img->width); j++) {
            fprintf(fh, "%d ", img->pixmap[i * img->width + j].r);
            fprintf(fh, "%d ", img->pixmap[i * img->width + j].g);
            fprintf(fh, "%d\n", img->pixmap[i * img->width + j].b);
        }
    }
    return 0;
}

/**
 * Writes ppm header struct information to a file stream
 * @param fh file handler
 * @param hdr header struct
 * @return 0 on success, -1 on error
 */
int write_header(FILE *fh, header *hdr) {
    int ret_val = 0;
    ret_val = fputs("P", fh);
    if (ret_val < 0) {
        return -1;
    }
    ret_val = fprintf(fh, "%d", hdr->file_type);
    if (ret_val < 0) {
        return -2;
    }
    ret_val = fputs("\n", fh);
    if (ret_val < 0) {
        return -3;
    }
    ret_val = fprintf(fh, "%d %d\n%d\n", hdr->width,
                                         hdr->height,
                                         hdr->max_color_val);
    if (ret_val < 0) {
        return -4;
    }
    return ret_val;
}


/* TESTING helper functions */
void print_pixels(RGBPixel *pixmap, int width, int height) {
    int i,j;
    int counter = 0;
    for (i=0; i<height; i++) {
        for (j=0; j<width; j++) {
            counter++;
            printf("r: %d, ", pixmap[i * width + j].r);
            printf("g: %d ,", pixmap[i * width + j].g);
            printf("b: %d\n", pixmap[i * width + j].b);
        }
    }
    printf("print_pixels count: %d\n", counter);
}