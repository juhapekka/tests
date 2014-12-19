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
} format_list[] = {
    { -1, "32bpp formats:" },
    { PIXMAN_a8r8g8b8, "PIXMAN_a8r8g8b8" },
    { PIXMAN_x8r8g8b8, "PIXMAN_x8r8g8b8" },
    { PIXMAN_a8b8g8r8, "PIXMAN_a8b8g8r8" },
    { PIXMAN_x8b8g8r8, "PIXMAN_x8b8g8r8" },
    { PIXMAN_b8g8r8a8, "PIXMAN_b8g8r8a8" },
    { PIXMAN_b8g8r8x8, "PIXMAN_b8g8r8x8" },
    { PIXMAN_r8g8b8a8, "PIXMAN_r8g8b8a8" },
    { PIXMAN_r8g8b8x8, "PIXMAN_r8g8b8x8" },
    { PIXMAN_x14r6g6b6, "PIXMAN_x14r6g6b6" },
    { PIXMAN_x2r10g10b10, "PIXMAN_x2r10g10b10" },
    { PIXMAN_a2r10g10b10, "PIXMAN_a2r10g10b10" },
    { PIXMAN_x2b10g10r10, "PIXMAN_x2b10g10r10" },
    { PIXMAN_a2b10g10r10, "PIXMAN_a2b10g10r10" },
    { -1, "sRGB formats:" },
    { PIXMAN_a8r8g8b8_sRGB, "PIXMAN_a8r8g8b8_sRGB" },
    { -1, "24bpp formats:" },
    { PIXMAN_r8g8b8, "PIXMAN_r8g8b8" },
    { PIXMAN_b8g8r8, "PIXMAN_b8g8r8" },
    { -1, "16bpp formats:" },
    { PIXMAN_r5g6b5, "PIXMAN_r5g6b5" },
    { PIXMAN_b5g6r5, "PIXMAN_b5g6r5" },
    { PIXMAN_a1r5g5b5, "PIXMAN_a1r5g5b5" },
    { PIXMAN_x1r5g5b5, "PIXMAN_x1r5g5b5" },
    { PIXMAN_a1b5g5r5, "PIXMAN_a1b5g5r5" },
    { PIXMAN_x1b5g5r5, "PIXMAN_x1b5g5r5" },
    { PIXMAN_a4r4g4b4, "PIXMAN_a4r4g4b4" },
    { PIXMAN_x4r4g4b4, "PIXMAN_x4r4g4b4" },
    { PIXMAN_a4b4g4r4, "PIXMAN_a4b4g4r4" },
    { PIXMAN_x4b4g4r4, "PIXMAN_x4b4g4r4" },
    { -1, "8bpp formats:" },
    { PIXMAN_a8, "PIXMAN_a8" },
    { PIXMAN_r3g3b2, "PIXMAN_r3g3b2" },
    { PIXMAN_b2g3r3, "PIXMAN_b2g3r3" },
    { PIXMAN_a2r2g2b2, "PIXMAN_a2r2g2b2" },
    { PIXMAN_a2b2g2r2, "PIXMAN_a2b2g2r2" },
    { PIXMAN_c8, "PIXMAN_c8" },
    { PIXMAN_g8, "PIXMAN_g8" },
    { PIXMAN_x4a4, "PIXMAN_x4a4" },
    { PIXMAN_x4c4, "PIXMAN_x4c4" },
    { PIXMAN_x4g4, "PIXMAN_x4g4" },
    { -1, "4bpp formats:" },
    { PIXMAN_a4, "PIXMAN_a4" },
    { PIXMAN_r1g2b1, "PIXMAN_r1g2b1" },
    { PIXMAN_b1g2r1, "PIXMAN_b1g2r1" },
    { PIXMAN_a1r1g1b1, "PIXMAN_a1r1g1b1" },
    { PIXMAN_a1b1g1r1, "PIXMAN_a1b1g1r1" },
    { PIXMAN_c4, "PIXMAN_c4" },
    { PIXMAN_g4, "PIXMAN_g4" },
    { -1, "1bpp formats:" },
    { PIXMAN_a1, "PIXMAN_a1" },
    { PIXMAN_g1, "PIXMAN_g1" },
    { -1, "YUV formats:" },
    { PIXMAN_yuy2, "PIXMAN_yuy2" },
    { PIXMAN_yv12, "PIXMAN_yv12" }
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
    int c, c2;
    fprintf(stderr, "accepted formats:");

    for(c = c2 = 0; c < sizeof(format_list)/sizeof(format_list[0]);
        c++ ) {

        if (format_list[c].format != -1 ) {
            fprintf(stderr, "%-19s", format_list[c].format_name );

            if (++c2 > 2) {
                c2 = 0;
                fprintf(stderr, "\n");
            }
        }
        else {
            fprintf(stderr, "\n\n%s\n", format_list[c].format_name );
            c2 = 0;
        }
    }
    fprintf(stderr, "\n\n");
}

int main( int argc, char *argv[] )
{
    int exitcode = EXIT_SUCCESS;
    int c;
    const int maxtests = looponce?1:0x7fffffff;
    struct timeval t1, t2;
    double elapsedTime;

    if (argc != 3) {
        fprintf(stderr, "-- simple pixman image format conversion performance"
                " test\ngive two parameters, source and target formats\n"
                "%s <source format> <target format>\n", argv[0] );

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
                PIXMAN_FORMAT_BPP(images[c].imgformat_pixman))-1)|3)+1;

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
    for (c = 0, elapsedTime = 0; elapsedTime < testseconds*1000.0f &&
         c < maxtests; c++) {
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
        elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.0f;
        elapsedTime += (t2.tv_usec - t1.tv_usec)/1000.0f;
    }

    fprintf(stderr, "%d times conversion from %s to %s format in %.3fs\n", c,
            format_list[images[test_source].selection_list_index].format_name,
            format_list[images[test_target].selection_list_index].format_name,
            elapsedTime/1000.0f);

away:
    for (c = 0; c < sizeof(images)/sizeof(images[0]); c++) {
        if (images[c].thisimage_pixman)
            pixman_image_unref(images[c].thisimage_pixman);

        free(images[c].imgdata.data);
    }
    return exitcode;
}
