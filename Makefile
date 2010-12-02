# upstream opencv builds. This variable selects this
OPENCV_DEBIAN_PACKAGES = 1

CXXFLAGS += -g -Wall -Wextra -pedantic -MMD -I../fltkVisionUtils `pkg-config --cflags cairo` -DSTUCK_FIREWIRE_WORKAROUND
LDFLAGS  += -g -lX11 -lXft -lXinerama

ifeq ($(OPENCV_DEBIAN_PACKAGES), 0)
  OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui
else
  CXXFLAGS += -DOPENCV_DEBIAN_PACKAGES
  OPENCV_LIBS = -lcv -lhighgui
endif

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += ../fltkVisionUtils/fltkVisionUtils.a -lfltk $(OPENCV_LIBS) -lpthread -ldc1394 $(FFMPEG_LIBS) `pkg-config --libs cairo`

all: makeFlipbook

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)

makeFlipbook: makeFlipbook.o generatePDF.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@


clean:
	rm -f *.o *.a *.d makeFlipbook

-include *.d
