
test:	bitcoin/*.c bcash/*.c support/*.c
	cc -I. -I./bitcoin -I./bcash -I./support -I./secp256k1 -o $@ $?

clean:
	rm -f *.o support/*.o test

run:	test
	./test
