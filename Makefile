CXXFLAGS += -g -Wall -Wextra -pedantic -MMD `pkg-config --cflags cairo`
LDFLAGS  += -g -lX11 -lXft -lXinerama

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += -lfltk -lcv -lpthread -ldc1394 $(FFMPEG_LIBS) -lfltkVisionUtils `pkg-config --libs cairo`

all: makeFlipbook

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)

makeFlipbook: makeFlipbook.o generatePDF.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@


clean:
	rm -f *.o *.a *.d makeFlipbook

-include *.d
