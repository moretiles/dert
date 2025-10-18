CFLAGS=-Wall -Wextra -Wpedantic --std=c11 -fwrapv
DEBUG=-g3 -ggdb
OPTIMIZE=

ANALYZE_GCC=-fanalyzer
ANALYZE_CLANG=-analyze-headers

#######################################################################
#                                                                     #
# If you want to actually use this library run `make all`             #
#                                                                     #
#######################################################################
all: libdert.a

clean:
	rm -f test tags *.ast *.pch *.plist *.o externalDefMap.txt gmon.out




## individual recipes
libdert.a: vstack.o vqueue.o vdll.o varena.o vpool.o varray.o vht.o fqueue.o cstring.o pointerarith.o
	ar rcs libdert.a *.o

vstack.o: vstack.c vstack.h
	${CC} ${OPTIMIZE} ${CFLAGS} vstack.c -c -o vstack.o

vqueue.o: vqueue.c vqueue.h
	${CC} ${OPTIMIZE} ${CFLAGS} vqueue.c -c -o vqueue.o

vdll.o: vdll.c vdll.h
	${CC} ${OPTIMIZE} ${CFLAGS} vdll.c -c -o vdll.o

varray.o: varray.c varray.h
	${CC} ${OPTIMIZE} ${CFLAGS} varray.c -c -o varray.o

vht.o: vht.c vht.h
	${CC} ${OPTIMIZE} ${CFLAGS} vht.c -c -o vht.o

varena.o: varena.c varena.h
	${CC} ${OPTIMIZE} ${CFLAGS} varena.c -c -o varena.o

vpool.o: vpool.c vpool.h
	${CC} ${OPTIMIZE} ${CFLAGS} vpool.c -c -o vpool.o

fqueue.o: fqueue.c fqueue.h
	${CC} ${OPTIMIZE} ${CFLAGS} fqueue.c -c -o fqueue.o

cstring.o: cstring.c cstring.h
	${CC} ${OPTIMIZE} ${CFLAGS} cstring.c -c -o cstring.o

pointerarith.o: pointerarith.c pointerarith.h
	${CC} ${OPTIMIZE} ${CFLAGS} pointerarith.c -c -o pointerarith.o

## tests and housekeeping
.PHONY: test
test:
	${CC} ${CFLAGS} ${DEBUG} -fsanitize=address vstack.c vqueue.c vdll.c varray.c varena.c vpool.c vht.c fqueue.c test.c pointerarith.c -o test

.PHONY: tags
tags:
	ctags -R .

# kind of a misnomer to test the performance of a "test build" but it's comparative data
performance: test
	./test
	gprof ./test gmon.out
