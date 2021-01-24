CC = cc
CFLAGS = -c -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-3.0` `pkg-config --cflags libavformat`
LDFLAGS = `pkg-config --libs gtk+-3.0` `pkg-config --libs libavformat` `pkg-config --libs libavcodec` `pkg-config --libs libavfilter` `pkg-config --libs libswscale` `pkg-config --libs libavutil`

PROG = HyperDeck-Controller
BINDIR = $(PROG)

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

PNG = $(wildcard Pixbufs/*.png)
PIXBUF = $(wildcard Pixbufs/*.h)

$(PROG): $(OBJS) Pixbufs/Pixbufs.o
	$(CC) -o $@ $^ $(LDFLAGS)

HyperDeck.o: HyperDeck.h HyperDeck_Protocol.h

Config.o: HyperDeck.h HyperDeck_Protocol.h

Preset.o: HyperDeck.h HyperDeck_Protocol.h

Transition.o: HyperDeck.h

Render_transition_8.o: HyperDeck.h

Render_transition_16.o: HyperDeck.h

Encode_Fresque.o: HyperDeck.h

Transcoding.o: HyperDeck.h

HyperDeck_Codec.o: HyperDeck.h

Fresque_Batch.o: HyperDeck.h

Fresque.o: HyperDeck.h HyperDeck_Protocol.h

File.o: HyperDeck.h HyperDeck_Protocol.h

HyperDeck_Protocol.o: HyperDeck.h HyperDeck_Protocol.h

Misc.o: HyperDeck.h

.c.o:
	$(CC) $(CFLAGS) $<

Pixbufs/Pixbufs.o: $(PNG) $(PIXBUF)
	@(cd Pixbufs && $(MAKE))

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

