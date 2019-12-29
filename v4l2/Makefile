CC= $(TOOLCHAIN_CC)

all : v4l2_test

v4l2_test : v4l2_test.o
	$(CC) -o v4l2_test v4l2_test.o

v4l2_test.o : v4l2_test.c
	$(CC) -c -o v4l2_test.o v4l2_test.c


clean:
	rm -rf *.o
	rm v4l2_test