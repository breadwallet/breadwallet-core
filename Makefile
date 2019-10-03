
test:	clean
	cc -I. -I./support -I./vendor/secp256k1 -o $@ bitcoin/*.c bcash/*.c ethereum/event/*.c support/*.c vendor/sqlite3/sqlite3.c

clean:
	rm -f *.o */*.o test

run:	test
	./test
