#ifndef __GENERAGE_PDF_HH__
#define __GENERAGE_PDF_HH__

#include <opencv/cv.h>

void generateFlipbook(const char* pdfFilename, IplImage const * const * frames);

#endif
