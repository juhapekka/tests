#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <pixman.h>
#include <GL/glu.h>
#include <sys/time.h>
#include <unistd.h>

unsigned int imgwidth = 512;
unsigned int imgheight = 512;
unsigned int testseconds = 1;

typedef struct {
    int width;
    int height;
    unsigned char *data;
} textureImage;


int singleBufferAttributess[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE,      1,   /* Request a single buffered color buffer */
    GLX_GREEN_SIZE,    1,   /* with the maximum number of color bits  */
    GLX_BLUE_SIZE,     1,   /* for each component                     */
    None
};

int doubleBufferAttributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER,  True,  /* Request a double-buffered color buffer with */
    GLX_RED_SIZE,      1,     /* the maximum number of bits per component    */
    GLX_GREEN_SIZE,    1,
    GLX_BLUE_SIZE,     1,
    None
};

struct
{
    float u, v, x, y, z;
} boxVertices[] =
{
    { 0.0f, 0.0f,    -1.0f,-1.0f, 0.0f },
    { 1.0f, 0.0f,     1.0f,-1.0f, 0.0f },
    { 1.0f, 1.0f,     1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f,    -1.0f, 1.0f, 0.0f }
};

GLuint  texture[1];
unsigned char* tempdata;
Display  *dpy;
GLXWindow glxWin;
int swapFlag = True;

static int pixman_texture_conversion(
        pixman_format_code_t srcFormat,
        uint32_t *srcBits, uint32_t srcStride, uint16_t width,
        uint16_t height, pixman_format_code_t dstFormat, uint32_t *dstBits,
        uint32_t dstStride)
{
     pixman_image_t *pixmanImage[2];

     if ((pixmanImage[0] = pixman_image_create_bits_no_clear(srcFormat,
                                                             width,
                                                             height,
                                                             srcBits,
                                                             srcStride))
             == NULL)
     {
         return 0;
     }

     /*
      * Assumption source and destination images are same size here.
      */
     if ((pixmanImage[1] = pixman_image_create_bits_no_clear(dstFormat,
                                                             width,
                                                             height,
                                                             dstBits,
                                                             dstStride))
             == NULL)
     {
         pixman_image_unref(pixmanImage[0]);
         return 0;
     }

     pixman_image_composite(PIXMAN_OP_SRC,
                            pixmanImage[0],
                            (pixman_image_t*) NULL,
                            pixmanImage[1],
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            width,
                            height);

     pixman_image_unref(pixmanImage[0]);
     pixman_image_unref(pixmanImage[1]);
     return 1;
}


static int ImageLoad(textureImage *image) {
    int size;
    int c1, c2;

    image->width = imgwidth;
    image->height = imgheight;

    size = image->height * image->width * 4;

    tempdata = (unsigned char *) malloc(size);
    if (tempdata == NULL) {
       printf("Error allocating memory for color-corrected image data");
       return 0;
    }

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

          tempdata[c1*imgwidth*4 + c2*4] = (char)((iteration%2)*128);
          tempdata[c1*imgwidth*4 + c2*4+1] = (char)((iteration%4)*64);
          tempdata[c1*imgwidth*4 + c2*4+2] = (char)((iteration%8)*32);
          tempdata[c1*imgwidth*4 + c2*4+3] = (char)-1;
        }
    }
    
    printf("Texture size %d x %d\n", image->width, image->height );

    image->data = (unsigned char *) malloc(size);
    return 1;
}

static Bool testGLTextures()
{
    Bool status;
    textureImage *texti;

    struct timeval t1, t2;
    double elapsedTime;
    unsigned int index, c;

    static const struct {
        char*                nimi;
        pixman_format_code_t pixmanformaatti_from;
        pixman_format_code_t pixmanformaatti;
        GLenum               texType;
        GLenum               texFormat;
        int                  bpp;
        int                  components;
    } testit[] = {
        { "BGRA8888, GL_UNSIGNED_BYTE", PIXMAN_a8r8g8b8, PIXMAN_a8r8g8b8, GL_BGRA, GL_UNSIGNED_BYTE, 4, GL_RGBA8 },
        { "BGRA8888, GL_UNSIGNED_INT_8_8_8_8", PIXMAN_a8r8g8b8, PIXMAN_b8g8r8a8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 4, GL_RGBA8 },
        { "GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV", PIXMAN_a8r8g8b8, PIXMAN_a8r8g8b8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, GL_RGBA8 },
        { "GL_RGB, GL_UNSIGNED_SHORT_5_6_5", PIXMAN_a8r8g8b8, PIXMAN_r5g6b5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 2, GL_RGB },
        { "GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV", PIXMAN_a8r8g8b8, PIXMAN_b5g6r5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, 2, GL_RGB },
        { "GL_RGB, GL_UNSIGNED_BYTE", PIXMAN_a8r8g8b8, PIXMAN_b8g8r8, GL_RGB, GL_UNSIGNED_BYTE, 3, GL_RGB8 },
        { "GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV", PIXMAN_a8r8g8b8, PIXMAN_a4r4g4b4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, GL_RGBA },
        { "GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4", PIXMAN_r8g8b8a8, PIXMAN_a4b4g4r4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, 2, GL_RGBA },
        { "GL_BGRA, GL_UNSIGNED_INT_10_10_10_2", PIXMAN_a8r8g8b8, PIXMAN_a2r10g10b10, GL_BGRA, GL_UNSIGNED_INT_10_10_10_2, 4, GL_RGBA }
    };

    status = False;
    texti = malloc(sizeof(textureImage));
    if (ImageLoad(texti))
    {
        status = True;
        glGenTextures(1, &texture[0]);
        glBindTexture(GL_TEXTURE_2D, texture[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        for (index = 0; index < sizeof(testit)/sizeof(testit[0]); index++ ) {
            pixman_texture_conversion( testit[index].pixmanformaatti_from,
                                       (uint32_t *)tempdata, imgwidth*4,
                                       imgwidth, imgheight,
                                       testit[index].pixmanformaatti,
                                       (uint32_t *)texti->data,
                                       imgwidth*testit[index].bpp);

            gettimeofday(&t1, NULL);
            for(c = 0, elapsedTime = 0; elapsedTime < testseconds*1000; c++) {
                glTexImage2D(GL_TEXTURE_2D, 0, testit[index].components,
                             texti->width, texti->height, 0,
                             testit[index].texType, testit[index].texFormat,
                             texti->data);
                             
                gettimeofday(&t2, NULL);
                elapsedTime = (t2.tv_sec - t1.tv_sec)*1000;
                elapsedTime += (t2.tv_usec - t1.tv_usec)/1000;
            }
            
            printf("%d times %s format conversion in %ds\n", c-1, testit[index].nimi, testseconds );

            glClearColor( (index&0xf)/16.0f, 1.0-(index&0x7)/8.0f, 0.0, 1.0 );
            glClear( GL_COLOR_BUFFER_BIT );

            glInterleavedArrays( GL_T2F_V3F, 0, boxVertices );
            glDrawArrays( GL_QUADS, 0, 4 );

            glFlush();

            if (swapFlag) {
                glXSwapBuffers( dpy, glxWin );
            }
        }
        glDeleteTextures(1, &texture[0]);
    }

    sleep( 3 );

    if (texti)
    {
        if (texti->data)
            free(texti->data);
        free(texti);
    }
    
    free(tempdata);
    return status;
}


static Bool WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
    return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
    (void)dpy;
}

int main( int argc, char *argv[] )
{
    Window                xWin;
    XEvent                event;
    XVisualInfo          *vInfo;
    XSetWindowAttributes  swa;
    GLXFBConfig          *fbConfigs;
    GLXContext            context;
    int                   swaMask;
    int                   numReturned;

    dpy = XOpenDisplay( NULL );
    if ( dpy == NULL ) {
        printf( "Unable to open a connection to the X server\n" );
        exit( EXIT_FAILURE );
    }

    fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
                                   doubleBufferAttributes, &numReturned );

    if ( fbConfigs == NULL ) {
      fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
                                     singleBufferAttributess, &numReturned );
      swapFlag = False;
    }

    vInfo = glXGetVisualFromFBConfig( dpy, fbConfigs[0] );

    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap( dpy, RootWindow(dpy, vInfo->screen),
                                    vInfo->visual, AllocNone );

    swaMask = CWBorderPixel | CWColormap | CWEventMask;

    xWin = XCreateWindow( dpy, RootWindow(dpy, vInfo->screen), 0, 0, 256, 256,
                          0, vInfo->depth, InputOutput, vInfo->visual,
                          swaMask, &swa );

    context = glXCreateNewContext( dpy, fbConfigs[0], GLX_RGBA_TYPE,
                 NULL, True );

    glxWin = glXCreateWindow( dpy, fbConfigs[0], xWin, NULL );

    XMapWindow( dpy, xWin );
    XIfEvent( dpy, &event, WaitForNotify, (XPointer) xWin );

    glXMakeContextCurrent( dpy, glxWin, glxWin, context );

    glEnable( GL_TEXTURE_2D );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective( 35.0f, 1.0f, 0.1f, 100.0f);

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0f, 0.0f, -5.0f );
    glRotatef( 0.0f, 1.0f, 0.0f, 0.0f );
    glRotatef( 0.0f, 0.0f, 1.0f, 0.0f );

    testGLTextures();

    glXMakeContextCurrent(dpy, 0, 0, 0);
    glXDestroyContext(dpy, context);
    glXDestroyWindow(dpy, glxWin);
    XDestroyWindow(dpy, xWin);
    XFree(vInfo);
    XCloseDisplay(dpy);
    exit( EXIT_SUCCESS );

    (void)argc;
    (void)argv;
}
