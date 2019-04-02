
test:	clean 
	cc -I. -I./support -I./secp256k1 -o $@ bitcoin/*.c bcash/*.c support/*.c

clean:
	rm -f *.o */*.o test

run:	test
	./test
