CFLAGS=-Wall -Wextra -Wpedantic --std=gnu11 -fwrapv -fmax-errors=5

INCLUDE=-I./header/ -I./SipHash/
TEST_INCLUDE=
TEST_INCLUDE=

GRAPHVIZ_LIB=-lcgraph -lgvc
LIB=-lm -luring
TEST_LIB=${GRAPHVIZ_LIB}

DEBUG=-g3 -gdwarf-4 -ggdb
OPTIMIZE=

ANALYZE_GCC=-fanalyzer
ANALYZE_CLANG=-analyze-headers

# Do not try to apply preprocesser to .S assembly files
.SUFFIXES: .S .s

#######################################################################
#                                                                     #
# If you want to actually use this library run `make all`             #
#                                                                     #
#######################################################################
all: libdert.a

# remove objects built by this library
clean:
	rm -f test tags *.ast *.pch *.plist obj/*.o externalDefMap.txt gmon.out

# remove objects built by this library and its dependencies
Clean:
	# required build files
	rm -f test tags *.ast *.pch *.plist obj/*.o externalDefMap.txt gmon.out

libdert.a: obj/siphash.o obj/vstack.o obj/vqueue.o obj/vdll.o obj/tbuf.o obj/varena.o obj/vpool.o obj/varray.o obj/vht.o obj/fqueue.o obj/cstring.o obj/aqueue.o obj/mpscqueue.o obj/tpoolrr.o obj/fmutex.o obj/fsemaphore.o obj/tree_T.o obj/tree_iterator.o obj/greent.o obj/greent_asm.o obj/pointerarith.o
	ar rcs libdert.a obj/*.o

## required dependency recipes
obj/siphash.o: .gitmodules
	[[ -d SipHash ]] || (echo 1>&2 "You need to follow the directions in README.MD" && exit 1)
	${CC} ${OPTIMIZE} ${CFLAGS} SipHash/siphash.c -c -o obj/siphash.o ${INCLUDE} ${LIB}

## individual recipes
obj/greent_asm.o: src/greent_asm.S header/greent*.h
	${CC} ${OPTIMIZE} ${CFLAGS} src/greent_asm.S -c -o obj/greent_asm.o ${INCLUDE} ${LIB}

obj/%.o: src/%.c header/%*.h
	${CC} ${OPTIMIZE} ${CFLAGS} $< -c -o $@ ${INCLUDE} ${LIB}

## tests and housekeeping
.PHONY: test test_asan test_tsan
test: libdert.a
	${CC} -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.c src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB}
	LSAN_OPTIONS=suppressions=sanitize-ignorelist.txt ./test

test_asan: libdert.a
	${CC} -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.c src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} -fsanitize=address
	LSAN_OPTIONS=suppressions=sanitize-ignorelist.txt ./test

test_tsan: libdert.a
	${CC} -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.S src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} -fsanitize=thread
	LSAN_OPTIONS=suppressions=sanitize-ignorelist.txt ./test

.PHONY: tags
tags:
	ctags -R .

# kind of a misnomer to test the performance of a "test build" but produces comparative data
performance: test
	./test
	gprof ./test gmon.out
