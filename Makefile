CC = cc
CFLAGS = -Wall -O2
LDFLAGS =

OBJS = fbpad.o term.o pad.o draw.o font.o isdw.o

all: fbpad mkfn_stb
fbpad.o: conf.h
term.o: conf.h
pad.o: conf.h
.c.o:
	$(CC) -c $(CFLAGS) $<

fbpad: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

mkfn_stb: mkfn_stb.c mkfn.o isdw.o
	$(CC) -c $(CFLAGS) -DSTB_TRUETYPE_IMPLEMENTATION mkfn_stb.c
	$(CC) -o $@ mkfn_stb.o mkfn.o isdw.o $(LDFLAGS) -lm

clean:
	rm -f *.o fbpad mkfn_stb
