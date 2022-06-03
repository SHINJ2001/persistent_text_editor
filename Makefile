all:  editor_interface.o appendbuffer.o headers.h main.o clean

editor_interface.o: editor_interface.c editor_interface.h
	cc -c editor_interface.c

appendbuffer.o: appendbuffer.c appendbuffer.h
	cc -c appendbuffer.c

main.o: main.c headers.h
	cc -c main.c
	cc main.o editor_interface.o appendbuffer.o -o editor

clean: 
	rm *.o
