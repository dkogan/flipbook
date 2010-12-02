#ifndef __GENERAGE_PDF_HH__
#define __GENERAGE_PDF_HH__

#include <opencv/cxcore.h>

void generateFlipbook(const char* pdfFilename, IplImage * const * frames);

#endif
