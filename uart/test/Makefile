CC= $(TOOLCHAIN_CC)

all : uart_test

uart_test : uart_test.o
	$(CC) -o uart_test uart_test.o

uart_test.o : uart_test.c
	$(CC) -c -o uart_test.o uart_test.c


clean:
	rm -rf *.o
	rm uart_test