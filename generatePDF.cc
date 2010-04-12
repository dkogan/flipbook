#include <cairo-pdf.h>
#include <stdio.h>
#include "layout.h"
#include "generatePDF.hh"

extern "C"
{
#include <libswscale/swscale.h>
}

// My openCV images store a pixel in 24 bits, while cairo expects 32 bits (despite the name of the
// format being CAIRO_FORMAT_RGB24). This is a class to convert my openCV data to that which is
// desired by cairo. At some point I maybe should update my fltk/opencv/ffmpeg library to handle
// RGB32 as a data type. This would move this conversion to the library and potentially speed things
// up by generating data in the correct format to begin with.
class PixelFormatConverter
{
public:
    SwsContext*    pSWSCtx;
    unsigned char* frameData;
    int            frameDataStride;

    PixelFormatConverter() : pSWSCtx(NULL), frameDataStride(NULL) {}
    void release(void)
    {
        if(pSWSCtx != NULL)
        {
            sws_freeContext(pSWSCtx);
            pSWSCtx = NULL;
        }
        if(frameData != NULL)
        {
            delete[] frameData;
            frameData = NULL;
        }
    }
    ~PixelFormatConverter()
    {
        release();
    }

    void convert(IplImage* frame)
    {
        if(pSWSCtx == NULL)
        {
            sws_getContext(IMAGE_W_PX, IMAGE_H_PX, PIX_FMT_RGB24,
                           IMAGE_W_PX, IMAGE_H_PX, PIX_FMT_RGB32,
                           SWS_POINT, NULL, NULL, NULL);
            if(pSWSCtx == NULL)
            {
                fprintf(stderr,
                        "Couldn't initialize the swscontext to convert images to a form acceptable to cairo\n");
                release();
                return;
            }

            frameDataStride = IMAGE_W_PX * 4;
            frameData = new unsigned char[frameDataStride * IMAGE_H_PX];
            if(frameData == NULL)
            {
                fprintf(stderr,
                        "couldn't allocate memory for the imager pixel format conversion buffer\n");
                release();
                return;
            }
        }

        sws_scale(pSWSCtx,
                  (uint8_t**)&frame->imageData, &frame->widthStep, 0, IMAGE_H_PX,
                  &frameData, &frameDataStride);
    }
} pixfmtConverter;

static void drawGrid(cairo_t* cr)
{
    for(int i=1; i<CELLS_PER_W; i++)
    {
        cairo_move_to(cr, CELL_W*i INCHES, 0);
        cairo_rel_line_to(cr, 0, PAGE_H INCHES);
    }

    for(int i=0; i<=CELLS_PER_H; i++)
    {
        cairo_move_to(cr, 0, ( PAGE_MARGIN_H + CELL_H*i ) INCHES);
        cairo_rel_line_to(cr, PAGE_W INCHES, 0);
    }
    cairo_stroke(cr);
}

void generateFlipbook(const char* pdfFilename, IplImage * const * frames)
{
    cairo_surface_t* pdf = cairo_pdf_surface_create(pdfFilename, PAGE_W INCHES, PAGE_H INCHES);
    cairo_t*         cr  = cairo_create (pdf);

    for(int sheet=0; sheet < SHEETS; sheet++)
    {
        drawGrid(cr);

        for(int cellCol = 0; cellCol < CELLS_PER_W; cellCol++)
        {
            for(int cellRow = 0; cellRow < CELLS_PER_H; cellRow++)
            {
                int cellIdx;
                if(cellCol == 0)
                {
                    cellIdx = (cellRow + 1          ) * SHEETS * CELLS_PER_W          - sheet - 1;
                    cairo_translate(cr, CELL_W INCHES, (PAGE_MARGIN_H + (cellRow + 0.5)*CELL_H) INCHES);
                }
                else
                {
                    cellIdx = (CELLS_PER_H - cellRow) * SHEETS * CELLS_PER_W - SHEETS - sheet - 1;
                    cairo_translate(cr, CELL_W INCHES, (PAGE_MARGIN_H + (cellRow + 0.5)*CELL_H) INCHES);
                    cairo_scale (cr, -1.0, -1.0);
                }

                cairo_move_to(cr, (-CELL_W + CELL_MARGIN_W/2.0) INCHES, 0);

                // My openCV images store a pixel in 24 bits, while cairo expects 32 bits (despite
                // the name of the format being CAIRO_FORMAT_RGB24). I thus convert my openCV data
                // to that which is desired by cairo. At some point I maybe should update my
                // fltk/opencv/ffmpeg library to handle RGB32 as a data type. This would move this
                // conversion to the library and potentially speed things up by generating data in
                // the correct format to begin with.
                pixfmtConverter.convert(frames[cellIdx]);

                cairo_surface_t* frame =
                    cairo_image_surface_create_for_data (pixfmtConverter.frameData,
                                                         CAIRO_FORMAT_RGB24,
                                                         IMAGE_W_PX, IMAGE_H_PX,
                                                         pixfmtConverter.frameDataStride);
                char str[16];
                sprintf(str, "frame %d", cellIdx);
                cairo_show_text(cr, str);

                cairo_scale (cr, SCALE_W, SCALE_H);
                cairo_set_source_surface (cr, frame,
                                          -IMAGE_W INCHES / SCALE_W,
                                          (-CELL_H / 2.0) INCHES / SCALE_H);

                cairo_paint (cr);

                cairo_identity_matrix(cr);
                cairo_surface_destroy (frame);
            }
        }
        cairo_surface_show_page(pdf);
    }

    cairo_destroy (cr);
    cairo_surface_destroy (pdf);
}
