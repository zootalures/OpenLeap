
LIBUSB_CFLAGS = $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LDFLAGS = $(shell pkg-config --libs libusb-1.0)

SDL_CFLAGS = $(shell pkg-config --cflags sdl)
SDL_LDFLAGS = $(shell pkg-config --libs sdl)

OPENCV_CFLAGS = $(shell pkg-config --cflags opencv)
OPENCV_LDFLAGS = $(shell pkg-config --libs opencv)

CFLAGS = -O2 -Wall  -g $(SDL_CFLAGS) $(OPENCV_CFLAGS) $(LIBUSB_CFLAGS) -DNDEBUG
LDFLAGS = $(LIBUSB_LDFLAGS)

CC = c99

.c.o: 
	$(CC) -c $(CFLAGS) $(LIBUSB_CFLAGS)  $<


all: low-level-leap-test display-leap-data-sdl display-leap-data-opencv bin/stereo_view

clean:
	rm -f *.o

leap_libusb_init.c.inc:
	@echo "Use make_leap_usbinit.sh to generate leap_libusb_init.c.inc."

low-level-leap.o: low-level-leap.c leap_libusb_init.c.inc


low-level-leap-test: low-level-leap.o low-level-leap-test.o
	$(CC) -o $@  $^ $(LDFLAGS) $(LIBUSB_LDFLAGS)


display-leap-data-sdl: display-leap-data-sdl.o
	$(CC) -o $@ $^ $(LDFLAGS) $(SDL_LDFLAGS)


display-leap-data-opencv: display-leap-data-opencv.o
	$(CC) -o $@ $^ $(LDFLAGS) $(OPENCV_LDFLAGS)


bin/stereo_view: stereo_view.o low-level-leap.o
	mkdir -p bin 
	$(CC) -o $@ $^ $(LDFLAGS) $(OPENCV_LDFLAGS) 


run: bin/stereo_view
	bin/stereo_view