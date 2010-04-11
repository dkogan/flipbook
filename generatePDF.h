#ifndef __GENERAGE_PDF_H__
#define __GENERAGE_PDF_H__

#include <opencv/cv.h>

void generateFlipbook(const char* pdfFilename, IplImage const * const * frames);

#endif
