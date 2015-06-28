LIB_TGT=libpsd.a

ARCH=x86
#CROSS=arm-none-linux-gnueabi-
CROSS=

CC=$(CROSS)gcc
CXX=$(CROSS)g++
STRIP=$(CROSS)strip
AR=$(CROSS)ar

CFLAGS=-Wall -I../inc -I./
LDFLAGS=

SLIB=-L ../lib/$(ARCH)
LIB=

OBJ=$(LIB_TGT)
LIB_OBJS=$(patsubst %.c,%.o,$(filter-out $(EXPORT_FILE),$(wildcard *.c)))

TIME_NOW=$(shell date "+%Y%m%d_%H%M%S")

all: $(OBJ) 

clean:
	-rm *.o $(OBJ)
	
%: %.c
	$(CC) $(CFLAGS) -o $@ $<  $(LIB)
	$(STRIP) $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.a: $(LIB_OBJS)
	$(AR) -rcvs $@ $^
	-cp -av $@ ../lib/$(ARCH)

%.so: $(LIB_OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^
