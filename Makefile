CC = cc
CFLAGS = -c -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-3.0` `pkg-config --cflags libavformat`
#CFLAGS = -c -Wno-error=incompatible-pointer-types -D_REENTRANT -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-3.0` `pkg-config --cflags libavformat`
LDFLAGS = `pkg-config --libs gtk+-3.0` `pkg-config --libs libavformat` `pkg-config --libs libavcodec` `pkg-config --libs libavfilter` `pkg-config --libs libswscale` `pkg-config --libs libavutil`

PROG = HyperDeck-Controller
BINDIR = $(PROG)

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

PNG = $(wildcard Pixbufs/*.png)
PIXBUF = $(wildcard Pixbufs/*.h)

$(PROG): $(OBJS) Pixbufs/Pixbufs.o
	$(CC) -o $@ $^ $(LDFLAGS)

Config.o: HyperDeck_Codec.h HyperDeck_Protocol.h Misc.h Osc.h Preset.h Render_Transition_8.h Render_Transition_16.h Transcoding.h Transition.h

Encode_Fresque.o: File.h Fresque_Batch.h HyperDeck_Codec.h Misc.h Transcoding.h Transition.h

File.o: Fresque.h HyperDeck_Codec.h HyperDeck_Protocol.h Misc.h Preset.h Transcoding.h

Fresque.o: File.h Fresque_Batch.h HyperDeck_Protocol.h

Fresque_Batch.o: File.h Fresque.h Misc.h HyperDeck_Codec.h Render_Transition_8.h Render_Transition_16.h Transition.h

HyperDeck.o: Config.h File.h Fresque.h Fresque_Batch.h HyperDeck_Codec.h HyperDeck_Protocol.h Misc.h Osc.h Pixbufs.h Preset.h Transcoding.h Transition.h

HyperDeck_Codec.o:

HyperDeck_Protocol.o: File.h Fresque.h Fresque_Batch.h HyperDeck_Codec.h Misc.h Pixbufs.h Preset.h

Logging.o: f_sync.h

Misc.o: Fresque.h Fresque_Batch.h HyperDeck_Codec.h Transcoding.h

Osc.o: Fresque.h HyperDeck_Protocol.h Pixbufs.h Preset.h

Pixbufs/Pixbufs.o: Pixbufs/Pixbufs.c Pixbufs.h $(PIXBUF) $(PNG)
	@(cd Pixbufs && $(MAKE))

Preset.o: Fresque.h Fresque_Batch.h HyperDeck_Protocol.h Pixbufs.h

Render_Transition_8.o: File.h HyperDeck_Codec.h Transcoding.h Transition.h

Render_Transition_16.o: File.h HyperDeck_Codec.h Transcoding.h Transition.h

Transcoding.o: Encode_Fresque.h File.h HyperDeck_Codec.h Misc.h Transition.h

Transition.o: File.h HyperDeck_Codec.h Preset.h Render_Transition_8.h Render_Transition_16.h

%.o: %.c %.h HyperDeck.h Logging.h
	$(CC) $(CFLAGS) $<

$(PROG).exe: $(OBJS) Pixbufs/Pixbufs.o Win32.o
	$(CC) -o $@ $^ $(LDFLAGS) -lwsock32

Win32.o: Win32/Win32.c
	$(CC) -c -Wall $<

install-win32: $(PROG).exe
	strip --strip-unneeded $(PROG).exe
	@mkdir -p c:/$(BINDIR)/Fresques
	cp -u $(PROG).exe Widgets.css HyperDeck.ico c:/$(BINDIR)
	@$(SHELL) Win32/install-gtk-dll $(BINDIR)

clean:
	rm -f *.o

clean-all:
	@(cd Pixbufs && $(MAKE) clean)
	rm -f *.o
	rm -f $(PROG)

