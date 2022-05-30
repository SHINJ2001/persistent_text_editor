all: main.o editor_interface.o appendbuffer.o headers.h
	cc main.o editor_interface.o appendbuffer.o -o editor

main.o: main.c headers.h
	cc -c main.c

editor_interface.o: editor_interface.c editor_interface.h
	cc -c editor_interface.c

appendbuffer.o: appendbuffer.c appendbuffer.h
	cc -c appendbuffer.c

clean: 
	rm *.o
