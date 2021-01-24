CC = cc
CFLAGS = -c -Wno-deprecated-declarations `pkg-config --cflags gtk+-3.0`

PNG = $(wildcard *.png)
PIXBUF = $(PNG:.png=.h)

.SUFFIXES: .png .h

Pixbufs.o: Pixbufs.c $(PIXBUF) $(PNG)
	$(CC) $(CFLAGS) $<

.png.h:
	gdk-pixbuf-csource --name=pixbuf_inline_$* --raw $< > $@

clean:
	rm -f *.h
	rm -f *.o

