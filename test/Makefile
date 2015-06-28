CROSS=
ARCH=x86

CC=$(CROSS)gcc
STRIP=$(CROSS)strip

LDFLAGS=
CFLAGS=-Wall

INCLUDE+= -I../src -I../inc
DLIBS=-lm
SLIBS=../lib/$(ARCH)/libpsd.a ../lib/$(ARCH)/libjpeg.a ../lib/$(ARCH)/libpng16.a ../lib/$(ARCH)/libz.a

EXEC = psdtest
OBJS = psdtest.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS) $(DLIBS) $(SLIBS)
	$(STRIP) $(EXEC)
$(OBJS):%.o: %.c
	$(CC) $(INCLUDE) $(CFLAG) $< -c

clean:
	rm -rf $(OBJS) $(EXEC)

