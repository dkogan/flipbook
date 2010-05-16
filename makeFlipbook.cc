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
#include <FL/Fl_File_Chooser.H>
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
    Fl_Button*        modeButton;
    Fl_Value_Slider*  videoPosition;
    Fl_Button*        makeBookButton;
} UIcontext;

enum FlipbookMode_t { streaming, recording, reviewing} mode;

static int numStoredFrames = 0;

static void doReview(void);
static void setLabelNumFrames(void);
static void doMakeBook(Fl_Widget* widget, void* cookie);

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

    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer
    if(mode == reviewing)
    {
        fprintf(stderr, "got new frame after stopStream(). Fix frameSource\n");
        return;
    }

    Fl::lock();
    {
        UIcontext.widgetImage->redrawNewFrame();
        if(mode == recording)
        {
            cvCopy(buffer, storedFrames[numStoredFrames++]);

            setLabelNumFrames();

            if(numStoredFrames == NUM_CELLS)
            {
                doReview();
                UIcontext.window->redraw();

                Fl::awake();
            }
        }
    }
    Fl::unlock();
}

static void setLabelNumFrames(void)
{
    char label[1024];
    snprintf(label, sizeof(label), "%d frames", numStoredFrames);
    UIcontext.videoPosition->copy_label(label);
}

static void doStream(void)
{
    mode = streaming;

    UIcontext.videoPosition->deactivate();
    UIcontext.videoPosition->value(0);

    UIcontext.makeBookButton->deactivate();

    UIcontext.modeButton->label("Start recording");

    numStoredFrames = 0;
    setLabelNumFrames();

    source->resumeStream();
}

static void doRecord(void)
{
    mode = recording;

    UIcontext.modeButton->label("Stop recording");
}

static void doReview(void)
{
    mode = reviewing;

    source->stopStream();

    if(numStoredFrames == 0)
    {
        UIcontext.videoPosition->deactivate();
        UIcontext.videoPosition->value(0);
    }
    else
    {
        UIcontext.videoPosition->activate();
        UIcontext.videoPosition->value(numStoredFrames-1);
        UIcontext.videoPosition->range(0, numStoredFrames-1);
    }

    UIcontext.makeBookButton->activate();

    UIcontext.modeButton->label("Clear");

    setLabelNumFrames();
}

static void doChangeMode(Fl_Widget* widget __attribute__((unused)), void* cookie __attribute__((unused)))
{
    switch(mode)
    {
    case streaming:
        doRecord();
        break;

    case recording:
        doReview();
        break;

    case reviewing:
        doStream();

    default: ;
    }
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

static void doMakeBook(Fl_Widget* widget __attribute__((unused)), void* cookie __attribute__((unused)))
{
    char* targetPath =  fl_file_chooser("Choose the flipbook output file",
                                        "*.pdf", NULL, 0);
    if(targetPath != NULL)
        generateFlipbook(targetPath, storedFrames);
}

int main(int argc, char* argv[])
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. If there's an argument, assume it's an input video. Otherwise, try
    // reading a camera
    if(argc >= 2) source = new FFmpegDecoder(argv[1], FRAMESOURCE_COLOR);
    else          source = new CameraSource(FRAMESOURCE_COLOR, true);

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

    UIcontext.window      = new Fl_Double_Window(source->w(), source->h() + BOX_H + 100, "Flipbook maker");
    UIcontext.widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                             WIDGET_COLOR);

    UIcontext.modeButton = new Fl_Button( 0, source->h(), BOX_W, BOX_H);
    UIcontext.modeButton->labelfont(FL_HELVETICA_BOLD);
    UIcontext.modeButton->callback(doChangeMode);

    UIcontext.videoPosition = new Fl_Value_Slider( BOX_W, source->h(), BOX_W, BOX_H);
    UIcontext.videoPosition->box(FL_DOWN_BOX);
    UIcontext.videoPosition->textsize(12);
    UIcontext.videoPosition->align(FL_ALIGN_RIGHT);
    UIcontext.videoPosition->type(FL_HOR_NICE_SLIDER);
    UIcontext.videoPosition->precision(0);
    UIcontext.videoPosition->step(1);
    UIcontext.videoPosition->callback(doVideoPosition);

    UIcontext.makeBookButton = new Fl_Button( BOX_W*2, source->h(), BOX_W, BOX_H, "Make flipbook!");
    UIcontext.makeBookButton->labelfont(FL_HELVETICA_BOLD);
    UIcontext.makeBookButton->labelsize(16);
    UIcontext.makeBookButton->deactivate();
    UIcontext.makeBookButton->callback(doMakeBook);

    UIcontext.window->resizable(UIcontext.window);
    UIcontext.window->end();
    UIcontext.window->show();


    // I'm starting a new camera-reading thread and storing the frame directly into the widget
    // buffer
    source->startSourceThread(&gotNewFrame, 0, *UIcontext.widgetImage);

    doStream();

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;
    delete UIcontext.widgetImage;
    delete UIcontext.modeButton;
    delete UIcontext.videoPosition;
    delete UIcontext.makeBookButton;
    delete UIcontext.window;

    for(int i=0; i<NUM_CELLS; i++)
        cvReleaseImage(&storedFrames[i]);

    return 0;
}
