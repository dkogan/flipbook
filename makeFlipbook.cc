#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <opencv/highgui.h>

#include "cvFltkWidget.hh"
#include "ffmpegInterface.hh"
#include "cameraSource.hh"
#include "layout.h"
#include "generatePDF.hh"

static FrameSource*  source;
static IplImage*     storedFrames[NUM_CELLS];

static struct
{
    Fl_Double_Window* window;
    CvFltkWidget*     widgetImage;
    Fl_Button*        stopRecord;
    Fl_Value_Slider*  videoPosition;
} UIcontext;

static int numStoredFrames = 0;



static void stopRecord_doStop(void);
static void setLabelNumFrames(void);

void gotNewFrame(IplImage* buffer __attribute__((unused)), uint64_t timestamp_us __attribute__((unused)))
{
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

    Fl::lock();
    setLabelNumFrames();

    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer
    UIcontext.widgetImage->redrawNewFrame();

    if(numStoredFrames == NUM_CELLS)
    {
        UIcontext.stopRecord->value(0);
        stopRecord_doStop();
        UIcontext.window->redraw();
        Fl::awake();
        //source->stopStream();
//         generateFlipbook("/tmp/tst.pdf", storedFrames);
    }

    Fl::unlock();
}

static void setLabelNumFrames(void)
{
    char label[1024];
    snprintf(label, sizeof(label), "%d frames", numStoredFrames);
    UIcontext.videoPosition->copy_label(label);
}

// THIS FUNCTION ASSUMES THAT IT IS CALLED INSIDE AN Fl::lock
static void stopRecord_doStop(void)
{
    // stopStream() may block to allow the last frame to be processed with gotNewFrame(). To avoid a
    // deadlock we unlock the FLTK mutex first. This function is called with this mutex held, so I
    // restore it after the stream has been stopped
    Fl::unlock();
    source->stopStream();
    Fl::lock();

    UIcontext.videoPosition->value(numStoredFrames > 0 ? numStoredFrames-1 : 0);
    UIcontext.stopRecord->labelcolor(FL_RED);
    UIcontext.stopRecord->label("Record new");
    if(numStoredFrames == 0)
        UIcontext.videoPosition->deactivate();
    else
    {
        UIcontext.videoPosition->activate();
        UIcontext.videoPosition->range(0, numStoredFrames-1);
    }

    setLabelNumFrames();
}

static void stopRecord_doRecord(void)
{
    UIcontext.stopRecord->labelcolor(FL_BLACK);
    UIcontext.stopRecord->label("Stop recording");
    UIcontext.videoPosition->deactivate();
    UIcontext.videoPosition->value(0);

    numStoredFrames = 0;
    source->resumeStream();
    setLabelNumFrames();
}

static void doStopRecord(Fl_Widget* widget __attribute__((unused)), void* cookie __attribute__((unused)))
{
    if(UIcontext.stopRecord->labelcolor() == FL_RED)
        stopRecord_doRecord();
    else
        stopRecord_doStop();
}

static void doVideoPosition(Fl_Widget* widget __attribute__((unused)), void* cookie __attribute__((unused)))
{
    int frame = (int)UIcontext.videoPosition->value();
    if(frame < 0 || frame >= numStoredFrames)
    {
        fprintf(stderr, "moved slider out of range. bug.\n");
        return;
    }

    cvCopy(storedFrames[frame], *UIcontext.widgetImage);
    UIcontext.widgetImage->redrawNewFrame();
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

#define BOX_W 150
#define BOX_H 20

    UIcontext.window      = new Fl_Double_Window(source->w(), source->h() + BOX_H + 100);
    UIcontext.widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                             WIDGET_COLOR);

    UIcontext.stopRecord = new Fl_Button( 0, source->h(), BOX_W, BOX_H);
    UIcontext.stopRecord->labelfont(FL_HELVETICA_BOLD);
    UIcontext.stopRecord->type(FL_TOGGLE_BUTTON);
    UIcontext.stopRecord->callback(doStopRecord);

    UIcontext.videoPosition = new Fl_Value_Slider( BOX_W, source->h(), BOX_W, BOX_H);
    UIcontext.videoPosition->box(FL_DOWN_BOX);
    UIcontext.videoPosition->textsize(12);
    UIcontext.videoPosition->align(FL_ALIGN_RIGHT);
    UIcontext.videoPosition->type(FL_HOR_NICE_SLIDER);
    UIcontext.videoPosition->precision(0);
    UIcontext.videoPosition->step(1);
    UIcontext.videoPosition->callback(doVideoPosition);

    stopRecord_doStop();

    UIcontext.window->resizable(UIcontext.window);
    UIcontext.window->end();
    UIcontext.window->show();


    // I'm starting a new camera-reading thread and storing the frame directly into the widget
    // buffer
    source->startSourceThread(&gotNewFrame, 0, *UIcontext.widgetImage);

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;
    delete UIcontext.widgetImage;
    delete UIcontext.stopRecord;
    delete UIcontext.videoPosition;
    delete UIcontext.window;

    for(int i=0; i<NUM_CELLS; i++)
        cvReleaseImage(&storedFrames[i]);

    return 0;
}
