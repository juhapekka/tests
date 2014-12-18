#include <stdio.h>
#include <stdlib.h>
#include <pixman.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>

#define imgwidth 512
#define imgheight 512
const int testseconds = 5;
const bool looponce = false;

/*
 * supported pixman format list for test:
 */
const struct {
    pixman_format_code_t    format;
    char*                   format_name;
    unsigned                format_bpp;
} format_list[] = {
    {PIXMAN_a8r8g8b8, "PIXMAN_a8r8g8b8", 4 },
    {PIXMAN_b8g8r8a8, "PIXMAN_b8g8r8a8", 4 },
    {PIXMAN_r8g8b8a8, "PIXMAN_r8g8b8a8", 4 },
    {PIXMAN_a2r10g10b10, "PIXMAN_a2r10g10b10", 4 },

    {PIXMAN_b8g8r8, "PIXMAN_b8g8r8", 3 },

    {PIXMAN_r5g6b5, "PIXMAN_r5g6b5", 2 },
    {PIXMAN_b5g6r5, "PIXMAN_b5g6r5", 2 },
    {PIXMAN_a4r4g4b4, "PIXMAN_a4r4g4b4", 2 },
    {PIXMAN_a4b4g4r4, "PIXMAN_a4b4g4r4", 2 }
};

typedef struct {
    unsigned width;
    unsigned height;
    unsigned char *data;
} textureImage;

struct {
    unsigned                selection_list_index;
    pixman_image_t          *thisimage_pixman;
    pixman_format_code_t    imgformat_pixman;
    unsigned                stride;
    textureImage            imgdata;
} images[] = {
    { 0, NULL, PIXMAN_a8r8g8b8, imgwidth*4, {imgwidth, imgheight, NULL} },
    { 0, NULL, 0, 0, {imgwidth, imgheight, NULL} },
    { 0, NULL, 0, 0, {imgwidth, imgheight, NULL} }
};

typedef enum {
    original = 0,
    test_source = 1,
    test_target = 2
} imagenames;


static bool image_load(const textureImage *image) {
    int c1, c2;

    for(c1 = 0; c1 < imgheight; c1++)
    {
      double y0 = ((((c1)*3.0)/imgheight)-1.5 );
        for(c2 = 0; c2 < imgwidth; c2++)
        {
          double x0 = ((((c2)*3.5)/imgwidth)-2.5 );
          double x = 0.0;
          double y = 0.0;

          int iteration = 0;
          int max_iteration = 1024;
          while ( x*x + y*y < 2*2  &&  iteration < max_iteration )
          {
            double xtemp = x*x - y*y + x0;
            y = 2*x*y + y0;
            x = xtemp;
            iteration = iteration + 1;
          }

          image->data[c1*imgwidth*4 + c2*4] = (char)((iteration%2)*128);
          image->data[c1*imgwidth*4 + c2*4+1] = (char)((iteration%4)*64);
          image->data[c1*imgwidth*4 + c2*4+2] = (char)((iteration%8)*32);
          image->data[c1*imgwidth*4 + c2*4+3] = (char)-1;
        }
    }
    return true;
}

static void print_accepted_formats()
{
    int c;
    fprintf(stderr, "accepted formats:\n");

    for(c = 0; c < sizeof(format_list)/sizeof(format_list[0]); c++ )
        fprintf(stderr, "%s\n", format_list[c].format_name );
}

int main( int argc, char *argv[] )
{
    int exitcode = EXIT_SUCCESS;
    int c;
    const int maxtests = looponce?1:0x7fffffff;
    struct timeval t1, t2;
    int elapsedTime;

    if (argc != 3) {
        fprintf(stderr, "-- simple pixman performance test\n"
                "give two parameters, source and target formats\n" );

        print_accepted_formats();

        exitcode = EXIT_FAILURE;
        goto away;
    }

    for(c = 0; c < sizeof(format_list)/sizeof(format_list[0]); c++ ) {
        if (strcmp(format_list[c].format_name, argv[1]) == 0)
            images[test_source].imgformat_pixman = format_list[c].format;

        if (strcmp(format_list[c].format_name, argv[2]) == 0)
            images[test_target].imgformat_pixman = format_list[c].format;
    }

    if (images[test_source].imgformat_pixman == 0) {
        fprintf(stderr, "Not supported format %s for source\n", argv[1] );

        print_accepted_formats();

        exitcode = EXIT_FAILURE;
        goto away;
    }

    if (images[test_target].imgformat_pixman == 0) {
        fprintf(stderr, "Not supported format %s for target\n", argv[2] );

        print_accepted_formats();

        exitcode = EXIT_FAILURE;
        goto away;
    }

    /*
     * allocate images
     */
    for (c = 0; c < sizeof(images)/sizeof(images[0]); c++) {
        /*
         * stride lenghts are divisible by 4
         */
        images[c].stride = (((images[c].imgdata.width *
                format_list[images[c].selection_list_index].format_bpp)
                -1)|3)+1;

        images[c].imgdata.data =
                (unsigned char *) malloc(images[c].imgdata.height *
                                         images[c].stride);

        if (images[c].imgdata.data == NULL) {
            exitcode = EXIT_FAILURE;
            goto away;
        }
    }

    /*
     * make complex test image
     */

    if (!image_load(&images[original].imgdata)) {
        exitcode = EXIT_FAILURE;
        goto away;
    }

    for (c = 0; c < sizeof(images)/sizeof(images[0]); c++) {
        if ((images[c].thisimage_pixman =
             pixman_image_create_bits_no_clear(images[c].imgformat_pixman,
                                               images[c].imgdata.width,
                                               images[c].imgdata.height,
                                               (uint32_t*)
                                               images[c].imgdata.data,
                                               images[c].stride))
                == NULL) {
            exitcode = EXIT_FAILURE;
            goto away;
        }
    }

    /*
     * Make our source image for test
     */
    pixman_image_composite(PIXMAN_OP_SRC,
                           images[original].thisimage_pixman,
                           (pixman_image_t*) NULL,
                           images[test_source].thisimage_pixman,
                           0, 0,
                           0, 0,
                           0, 0,
                           images[original].imgdata.width,
                           images[original].imgdata.height);

    /*
     * run timeloop test
     */
    gettimeofday(&t1, NULL);
    for (c = 0, elapsedTime = 0; elapsedTime < testseconds && c < maxtests;
         c++) {
        pixman_image_composite(PIXMAN_OP_SRC,
                               images[test_source].thisimage_pixman,
                               (pixman_image_t*) NULL,
                               images[test_target].thisimage_pixman,
                               0, 0,
                               0, 0,
                               0, 0,
                               images[original].imgdata.width,
                               images[original].imgdata.height);

        gettimeofday(&t2, NULL);
        elapsedTime = t2.tv_sec - t1.tv_sec;
    }

    fprintf(stderr, "%d times conversion from %s to %s format in %ds\n", c-1,
            format_list[images[test_source].selection_list_index].format_name,
            format_list[images[test_target].selection_list_index].format_name,
            elapsedTime);

away:
    for (c = 0; c < sizeof(images)/sizeof(images[0]); c++) {
        if (images[c].thisimage_pixman)
            pixman_image_unref(images[c].thisimage_pixman);

        free(images[c].imgdata.data);
    }
    return exitcode;
}
