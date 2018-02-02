intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib malloc32.o
	gcc $(CFLAGS) -std=c99 -fpic -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib64 malloc64.o
	gcc $(CFLAGS) -std=c99 -fpic -m64 -shared -o $@ malloc64.o

lib:
	mkdir lib

lib64:
	mkdir lib64

malloc32.o: malloc.c
	gcc $(CFLAGS) -std=c99 -fpic -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	gcc $(CFLAGS) -std=c99 -fpic -m64 -c -o malloc64.o malloc.c

clean:
	rm -f *.o *.a
