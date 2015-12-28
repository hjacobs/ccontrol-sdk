#selfhacked makefile for ccontrolSDK
# by Diman Todorov
PROJ = ccontrolSDK




CCBASICOBJS=ccbasic/alias.o ccbasic/basiscan.o ccbasic/ccbasic.o ccbasic/compiler.o\
     ccbasic/compscan.o ccbasic/morio.o ccbasic/scanterm.o ccbasic/textres.o

ASMOBJS=asm/asXX.o

UOBJS=updown/ccupload.o
DOBJS=updown/ccdownload.o

DNIXOBJS=tools/dos2unix.o

CPP=g++
CC=gcc
AS=as
RM=rm
.SUFFIXES : .o .c .s

CCFLAGS = -Wall -ansi -g


CCBASIC=bin/ccbasic
ASM=bin/ccasm
UP=bin/ccupload
DOWN=bin/ccdownload
DNIX=bin/dos2unix

all: ccbasic asm ud dos2unix

ccbasic: $(CCBASICOBJS)
	$(CPP) -o $(CCBASIC) $(CCBASICOBJS) 

asm: $(ASMOBJS)
	$(CC) $(CFLAGS) -o $(ASM) $(ASMOBJS)

ud: $(UOBJS) $(DOBJS)
	$(CC) $(CFLAGS) -o $(UP) $(UOBJS)
	$(CC) $(CFLAGS) -o $(DOWN) $(DOBJS)

dos2unix: $(DNIXOBJS) 
	$(CC) $(CFLAGS) -o $(DNIX) $(DNIXOBJS)

clean: 
	rm $(ASMOBJS)
	rm $(CCBASICOBJS)
	rm $(UOBJS) $(DOBJS)
	rm $(DNIXOBJS)
	rm $(ASM)	
	rm $(CCBASIC)
	rm $(UP) $(DOWN)
	rm $(DNIX)
