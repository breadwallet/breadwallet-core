
test:	*.c support/*.c
	cc -I. -I./support -I./secp256k1 -o $@ $?

clean:
	rm -f *.o support/*.o test

run:	test
	./test
