#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#define SHEETS        6
#define PAGE_W        8.5
#define PAGE_H        11
#define PAGE_MARGIN_H 0.5

#define ASPECT        (4.0/3.0)
#define CELLS_PER_H   7
#define CELLS_PER_W   2
#define CELL_H        ( (PAGE_H - 2.0*PAGE_MARGIN_H ) / CELLS_PER_H )
#define CELL_W        ( PAGE_W / CELLS_PER_W )
#define CELL_MARGIN_W ( CELL_W - CELL_H * ASPECT )
#define IMAGE_W       ( CELL_W - CELL_MARGIN_W )
#define IMAGE_H       ( CELL_H )

#define NUM_CELLS     ( CELLS_PER_H * CELLS_PER_W * SHEETS )

#define INCHES *72 /* points per inch */

#define IMAGE_W_PX 640
#define IMAGE_H_PX 480

#define SCALE_W    (IMAGE_W INCHES / IMAGE_W_PX)
#define SCALE_H    (IMAGE_H INCHES / IMAGE_H_PX)


#endif
