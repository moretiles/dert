CFLAGS=-Wall -Wextra --std=c11
DEBUG=-g3 -ggdb
OPTIMIZE=-O3

ANALYZE_GCC=-fanalyzer
ANALYZE_CLANG=-analyze-headers

#######################################################################
#                                                                     #
# If you want to actually use this library run `make all`             #
#                                                                     #
#######################################################################
all: cstring.o vdll.o vpool.o




## individual recipes
cstring.o: cstring.c cstring.h
	${CC} ${CFLAGS} ${OPTIMIZE} cstring.c -c -o cstring.o

vdll.o: vdll.c vdll.h
	${CC} ${CFLAGS} ${OPTIMIZE} vdll.c -c -o vdll.o

vpool.o: vpool.c vpool.h
	${CC} ${CFLAGS} ${OPTIMIZE} vpool.c -c -o vpool.o

clean:
	rm -f test tags *.ast *.pch *.plist *.o externalDefMap.txt gmon.out




## tests and housekeeping
test:
	${CC} ${CFLAGS} ${DEBUG} -fsanitize=address vdll.c vpool.c test.c -o test

.PHONY: tags
tags:
	ctags -R .

# kind of a misnomer to test the performance of a "test build" but it's comparative data
performance: test
	./test
	gprof ./test gmon.out
