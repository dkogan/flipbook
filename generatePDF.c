#include <cairo-pdf.h>
#include <stdio.h>
#include "layout.h"


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

int main (void)
{
    cairo_surface_t* pdf = cairo_pdf_surface_create("/tmp/tst.pdf", PAGE_W INCHES, PAGE_H INCHES);
    cairo_t*         cr  = cairo_create (pdf);
    cairo_surface_t* png = cairo_image_surface_create_from_png("00000001.png");

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
                char str[1024];
                sprintf(str, "frame %d", cellIdx);
                cairo_show_text(cr, str);



                cairo_scale (cr, SCALE_W, SCALE_H);
                cairo_set_source_surface (cr, png,
                                          -IMAGE_W INCHES / SCALE_W,
                                          (-CELL_H / 2.0) INCHES / SCALE_H);

                cairo_paint (cr);

                cairo_identity_matrix(cr);
            }
        }
        cairo_surface_show_page(pdf);
    }

    cairo_surface_destroy (png);
    cairo_destroy (cr);
    cairo_surface_destroy (pdf);

    return 0;
}
