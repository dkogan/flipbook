CXXFLAGS += -g -Wall -Wextra -pedantic -MMD
LDFLAGS  += -g -lX11 -lXft -lXinerama

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += -lfltk -lcv -lpthread -ldc1394 $(FFMPEG_LIBS) -lfltkVisionUtils

all: makeFlipbook

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)

makeFlipbook: makeFlipbook.o generatePDF.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@


clean:
	rm -f *.o *.a *.d makeFlipbook

-include *.d
