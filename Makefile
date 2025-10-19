CFLAGS=-Wall -Wextra -Wpedantic --std=c11 -fwrapv
INCLUDE=-Iheader -ISipHash
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
	rm -f test tags *.ast *.pch *.plist obj/*.o externalDefMap.txt gmon.out



libdert.a: obj/vstack.o obj/vqueue.o obj/vdll.o obj/varena.o obj/vpool.o obj/varray.o obj/vht.o obj/fqueue.o obj/cstring.o obj/pointerarith.o obj/siphash.o
	ar rcs libdert.a obj/*.o

## individual recipes
obj/vstack.o: src/vstack.c header/vstack.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/vstack.c -c -o obj/vstack.o ${INCLUDE}

obj/vqueue.o: src/vqueue.c header/vqueue.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/vqueue.c -c -o obj/vqueue.o ${INCLUDE}

obj/vdll.o: src/vdll.c header/vdll.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/vdll.c -c -o obj/vdll.o ${INCLUDE}

obj/varray.o: src/varray.c header/varray.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/varray.c -c -o obj/varray.o ${INCLUDE}

obj/vht.o: src/vht.c header/vht.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/vht.c -c -o obj/vht.o ${INCLUDE}

obj/varena.o: src/varena.c header/varena.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/varena.c -c -o obj/varena.o ${INCLUDE}

obj/vpool.o: src/vpool.c header/vpool.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/vpool.c -c -o obj/vpool.o ${INCLUDE}

obj/fqueue.o: src/fqueue.c header/fqueue.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/fqueue.c -c -o obj/fqueue.o ${INCLUDE}

obj/cstring.o: src/cstring.c header/cstring.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/cstring.c -c -o obj/cstring.o ${INCLUDE}

obj/pointerarith.o: src/pointerarith.c header/pointerarith.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/pointerarith.c -c -o obj/pointerarith.o ${INCLUDE}

## dependency recipes
obj/siphash.o: SipHash/siphash.c SipHash/siphash.h
	${CC} ${OPTIMIZE} ${CFLAGS} SipHash/siphash.c -c -o obj/siphash.o ${INCLUDE}

## tests and housekeeping
.PHONY: test
test:
	${CC} ${CFLAGS} ${DEBUG} -fsanitize=address src/vstack.c src/vqueue.c src/vdll.c src/varray.c src/varena.c src/vpool.c SipHash/siphash.c src/vht.c src/fqueue.c src/test.c src/pointerarith.c -o test ${INCLUDE}

.PHONY: tags
tags:
	ctags -R .

# kind of a misnomer to test the performance of a "test build" but produces comparative data
performance: test
	./test
	gprof ./test gmon.out
