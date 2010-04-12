#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <opencv/highgui.h>

#include "cvFltkWidget.hh"
#include "ffmpegInterface.hh"
#include "cameraSource.hh"
#include "layout.h"
#include "generatePDF.hh"

static FrameSource*  source;
static CvFltkWidget* widgetImage;
static IplImage*     storedFrames[NUM_CELLS];

static int numStoredFrames = 0;

void gotNewFrame(IplImage* buffer __attribute__((unused)), uint64_t timestamp_us __attribute__((unused)))
{
    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer

    Fl::lock();
    widgetImage->redrawNewFrame();
    Fl::unlock();

    // I should only be getting new frames if I need more data. If I have all the data I need AND I
    // got a new frame, something is wrong. The below are not assertions because I want to separate
    // the two error cases
    if(numStoredFrames == NUM_CELLS)
    {
        fprintf(stderr, "ERROR: gotNewFrames() when numStoredFrames == NUM_CELLS!\n");
        return;
    }
    if(numStoredFrames > NUM_CELLS)
    {
        fprintf(stderr, "ERROR: gotNewFrames() when numStoredFrames > NUM_CELLS!\n");
        return;
    }

    cvCopy(buffer, storedFrames[numStoredFrames++]);

    if(numStoredFrames == NUM_CELLS)
    {
        source->stopStream();
        generateFlipbook("/tmp/tst.pdf", storedFrames);
    }
}

int main(int argc, char* argv[])
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. If there's an argument, assume it's an input video. Otherwise, try
    // reading a camera
    if(argc >= 2) source = new FFmpegDecoder(argv[1], FRAMESOURCE_COLOR);
    else          source = new CameraSource(FRAMESOURCE_COLOR);

    if(! *source)
    {
        fprintf(stderr, "couldn't open source\n");
        delete source;
        return 0;
    }

    for(int i=0; i<NUM_CELLS; i++)
    {
        storedFrames[i] = cvCreateImage(cvSize(source->w(), source->h()), IPL_DEPTH_8U, 3);
        if(storedFrames[i] == NULL)
        {
            fprintf(stderr, "couldn't allocate memory for all the frames\n");
            for(i--; i>=0; i--)
                cvReleaseImage(&storedFrames[i]);
            delete source;
            return 0;
        }
    }

    Fl_Window window(source->w(), source->h());
    widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                   WIDGET_COLOR);

    window.resizable(window);
    window.end();
    window.show();

    // I'm starting a new camera-reading thread and storing the frame directly into the widget
    // buffer
    source->startSourceThread(&gotNewFrame, SOURCE_PERIOD_US, *widgetImage);

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;
    delete widgetImage;

    for(int i=0; i<NUM_CELLS; i++)
        cvReleaseImage(&storedFrames[i]);

    return 0;
}
