OBJ_FSP = fsp.o  frag.o  mia_parse.o
CC = gcc  -std=c99 -m64 -O2 -Wall -fopenmp
CFLAGS = -DFSP -DEFEDT_G_IS_UINT32 -DOPENMP -DLIIAR  -DLIBPROJ -DTESTING -DTEST2 -DEERIE -DUNIX -DMARCIN  -DDISK_WRITE -Wall -w

fsp:  fsp.o  frag.o  mia_parse.o
	gcc -fopenmp -o fsp fsp.o frag.o mia_parse.o libmiallib_generic.a -lm -ldl -lpthread -lgomp -lgsl -ltiff; cp fsp mspa_lin64;

%.o: %.c liiar.h
	$(COMPILE.c) -o $@ $<

clean:
	rm -f *.o; rm -f mspa_lin64 fsp
