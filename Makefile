CFLAGS=-Wall -Wextra -Wpedantic -Wno-parentheses --std=gnu11 -fwrapv -fmax-errors=5
CXXFLAGS=-Wall -Wextra -Wpedantic -Wno-parentheses -Wno-missing-field-initializers --std=gnu++17 -fwrapv -fms-extensions -fmax-errors=5

INCLUDE=-I./header/ -I./SipHash/ -I./tests/src/
TEST_INCLUDE=
TEST_INCLUDE=

GRAPHVIZ_LIB=-lcgraph -lgvc
LIB=-lm -luring
TEST_LIB=${GRAPHVIZ_LIB}

DEBUG=-g3 -gdwarf-4 -ggdb -DMT_TLD=
OPTIMIZE=-O3

ASAN=-fsanitize=address -fno-sanitize-address-use-after-scope
TSAN=-fsanitize=thread
LSAN_SUPPRESSIONS=LSAN_OPTIONS=suppressions=sanitizer/leak-sanitizer-ignorelist.txt
TSAN_SUPPRESSIONS=TSAN_OPTIONS=suppressions=sanitizer/thread-sanitizer-ignorelist.txt

ANALYZE_GCC=-fanalyzer
ANALYZE_CLANG=-analyze-headers

OBJS=obj/siphash.o obj/vstack.o obj/vqueue.o obj/vdll.o obj/tbuf.o obj/varena.o obj/vpool.o obj/varray.o obj/vht.o obj/fqueue.o obj/cstring.o obj/aqueue.o obj/mpscqueue.o obj/tpoolrr.o obj/gtpoolrr.o obj/fmutex.o obj/fsemaphore.o obj/tree_T.o obj/tree_iterator.o obj/tree_iterator_pre.o obj/tree_iterator_in.o obj/tree_iterator_post.o obj/tree_iterator_bfs.o obj/greent.o obj/greent_asm.o obj/pointerarith.o obj/tld.o
OBJS_TEST=tests/obj/test/siphash.o tests/obj/test/vstack.o tests/obj/test/vqueue.o tests/obj/test/vdll.o tests/obj/test/tbuf.o tests/obj/test/varena.o tests/obj/test/vpool.o tests/obj/test/varray.o tests/obj/test/vht.o tests/obj/test/fqueue.o tests/obj/test/cstring.o tests/obj/test/aqueue.o tests/obj/test/mpscqueue.o tests/obj/test/tpoolrr.o tests/obj/test/gtpoolrr.o tests/obj/test/fmutex.o tests/obj/test/fsemaphore.o tests/obj/test/tree_T.o tests/obj/test/tree_iterator.o tests/obj/test/tree_iterator_pre.o tests/obj/test/tree_iterator_in.o tests/obj/test/tree_iterator_post.o tests/obj/test/tree_iterator_bfs.o tests/obj/test/greent.o tests/obj/test/greent_asm.o tests/obj/test/pointerarith.o tests/obj/test/tld.o
OBJS_TEST_ASAN=tests/obj/test_asan/siphash.o tests/obj/test_asan/vstack.o tests/obj/test_asan/vqueue.o tests/obj/test_asan/vdll.o tests/obj/test_asan/tbuf.o tests/obj/test_asan/varena.o tests/obj/test_asan/vpool.o tests/obj/test_asan/varray.o tests/obj/test_asan/vht.o tests/obj/test_asan/fqueue.o tests/obj/test_asan/cstring.o tests/obj/test_asan/aqueue.o tests/obj/test_asan/mpscqueue.o tests/obj/test_asan/tpoolrr.o tests/obj/test_asan/gtpoolrr.o tests/obj/test_asan/fmutex.o tests/obj/test_asan/fsemaphore.o tests/obj/test_asan/tree_T.o tests/obj/test_asan/tree_iterator.o tests/obj/test_asan/tree_iterator_pre.o tests/obj/test_asan/tree_iterator_in.o tests/obj/test_asan/tree_iterator_post.o tests/obj/test_asan/tree_iterator_bfs.o tests/obj/test_asan/greent.o tests/obj/test_asan/greent_asm.o tests/obj/test_asan/pointerarith.o tests/obj/test_asan/tld.o
OBJS_TEST_TSAN=tests/obj/test_tsan/siphash.o tests/obj/test_tsan/vstack.o tests/obj/test_tsan/vqueue.o tests/obj/test_tsan/vdll.o tests/obj/test_tsan/tbuf.o tests/obj/test_tsan/varena.o tests/obj/test_tsan/vpool.o tests/obj/test_tsan/varray.o tests/obj/test_tsan/vht.o tests/obj/test_tsan/fqueue.o tests/obj/test_tsan/cstring.o tests/obj/test_tsan/aqueue.o tests/obj/test_tsan/mpscqueue.o tests/obj/test_tsan/tpoolrr.o tests/obj/test_tsan/gtpoolrr.o tests/obj/test_tsan/fmutex.o tests/obj/test_tsan/fsemaphore.o tests/obj/test_tsan/tree_T.o tests/obj/test_tsan/tree_iterator.o tests/obj/test_tsan/tree_iterator_pre.o tests/obj/test_tsan/tree_iterator_in.o tests/obj/test_tsan/tree_iterator_post.o tests/obj/test_tsan/tree_iterator_bfs.o tests/obj/test_tsan/greent.o tests/obj/test_tsan/greent_asm.o tests/obj/test_tsan/pointerarith.o tests/obj/test_tsan/tld.o

# Do not try to apply preprocesser to .S assembly files
.SUFFIXES: .S .s

#######################################################################
#                                                                     #
# If you want to actually use this library run `make all`             #
#                                                                     #
#######################################################################
all: libdert.a

clean:
	rm -f tags *.ast *.pch *.plist obj/*.o externalDefMap.txt gmon.out
	rm -f test tests/obj/test/*.o tests/lib/test/*.a
	rm -f test_asan tests/obj/test_asan/*.o tests/lib/test_asan/*.a
	rm -f test_tsan tests/obj/test_tsan/*.o tests/lib/test_tsan/*.a

libdert.a: ${OBJS}
	ar rcs libdert.a obj/*.o

obj/siphash.o: .gitmodules
	[[ -d SipHash ]] || (echo 1>&2 "You need to follow the directions in README.MD" && exit 1)
	gcc ${OPTIMIZE} ${CFLAGS} SipHash/siphash.c -c -o obj/siphash.o ${INCLUDE} ${LIB}

obj/greent_asm.o: src/greent_asm.S header/greent*.h
	gcc ${OPTIMIZE} ${CFLAGS} src/greent_asm.S -c -o obj/greent_asm.o ${INCLUDE} ${LIB}

obj/%.o: src/%.c header/%*.h
	gcc ${OPTIMIZE} ${CFLAGS} $< -c -o $@ ${INCLUDE} ${LIB}

#######################################################################
#                                                                     #
# Everything below here is for tests                                  #
#                                                                     #
#######################################################################

## NORMAL TESTS ##

test: tests/lib/test/libdert.a tests/src/*.cc
	g++ -DDERT_TEST=1 ${CXXFLAGS} ${DEBUG} tests/src/test.cc -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} -lgtest -lgtest_main -L./tests/lib/test/ -ldert 
	./test

tests/lib/test/libdert.a: ${OBJS_TEST}
	ar rcs tests/lib/test/libdert.a tests/obj/test/*.o

tests/obj/test/siphash.o: .gitmodules
	[[ -d SipHash ]] || (echo 1>&2 "You need to follow the directions in README.MD" && exit 1)
	gcc ${DEBUG} ${CFLAGS} SipHash/siphash.c -c -o tests/obj/test/siphash.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB}

tests/obj/test/greent_asm.o: src/greent_asm.S header/greent*.h
	gcc ${DEBUG} ${CFLAGS} src/greent_asm.S -c -o tests/obj/test/greent_asm.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB}

tests/obj/test/%.o: src/%.c header/%*.h
	gcc ${DEBUG} ${CFLAGS} $< -c -o $@ ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB}

## ASAN TESTS ##

test_asan: tests/lib/test_asan/libdert.a tests/src/*.cc
	g++ -DDERT_TEST=1 ${CXXFLAGS} ${DEBUG} tests/src/test.cc -o test_asan ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} -lgtest -lgtest_main -L./tests/lib/test_asan/ -ldert ${ASAN}
	${LSAN_SUPPRESSIONS} ./test_asan

tests/lib/test_asan/libdert.a: ${OBJS_TEST_ASAN}
	ar rcs tests/lib/test_asan/libdert.a tests/obj/test_asan/*.o

tests/obj/test_asan/siphash.o: .gitmodules
	[[ -d SipHash ]] || (echo 1>&2 "You need to follow the directions in README.MD" && exit 1)
	gcc ${DEBUG} ${CFLAGS} SipHash/siphash.c -c -o tests/obj/test_asan/siphash.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${ASAN}

tests/obj/test_asan/greent_asm.o: src/greent_asm.S header/greent*.h
	gcc ${DEBUG} ${CFLAGS} src/greent_asm.S -c -o tests/obj/test_asan/greent_asm.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${ASAN}

tests/obj/test_asan/%.o: src/%.c header/%*.h
	gcc ${DEBUG} ${CFLAGS} $< -c -o $@ ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${ASAN}

## TSAN TESTS ##

test_tsan: tests/lib/test_tsan/libdert.a tests/src/*.cc
	g++ -DDERT_TEST=1 ${CXXFLAGS} ${DEBUG} tests/src/test.cc -o test_tsan ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} -lgtest -lgtest_main -L./tests/lib/test_tsan/ -ldert ${TSAN}
	${LSAN_SUPPRESSIONS} ${TSAN_SUPPRESSIONS} ./test_tsan

tests/lib/test_tsan/libdert.a: ${OBJS_TEST_TSAN}
	ar rcs tests/lib/test_tsan/libdert.a tests/obj/test_tsan/*.o

tests/obj/test_tsan/siphash.o: .gitmodules
	[[ -d SipHash ]] || (echo 1>&2 "You need to follow the directions in README.MD" && exit 1)
	gcc ${DEBUG} ${CFLAGS} SipHash/siphash.c -c -o tests/obj/test_tsan/siphash.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${TSAN}

tests/obj/test_tsan/greent_asm.o: src/greent_asm.S header/greent*.h
	gcc ${DEBUG} ${CFLAGS} src/greent_asm.S -c -o tests/obj/test_tsan/greent_asm.o ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${TSAN}

tests/obj/test_tsan/%.o: src/%.c header/%*.h
	gcc ${DEBUG} ${CFLAGS} $< -c -o $@ ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${TSAN}

## tests and housekeeping
.PHONY: test test_asan test_tsan
#test: libdert.a
	#gcc -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.c src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB}
	#./test

#test_asan: libdert.a
	#gcc -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.c src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${ASAN}
	#${LSAN_SUPPRESSIONS} ./test

#test_tsan: libdert.a
	#gcc -DDERT_TEST=1 ${CFLAGS} ${DEBUG} src/*.c src/*.S SipHash/siphash.c -o test ${INCLUDE} ${TEST_INCLUDE} ${LIB} ${TEST_LIB} ${TSAN}
	#${LSAN_SUPPRESSIONS} ${TSAN_SUPPRESSIONS} ./test

.PHONY: tags
tags:
	ctags -R .

# kind of a misnomer to test the performance of a "test build" but produces comparative data
performance: test
	./test
	gprof ./test gmon.out
