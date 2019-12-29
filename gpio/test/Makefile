CC= $(TOOLCHAIN_CC)
INC= -I ../driver

all : am335x_gpio_test

am335x_gpio_test : am335x_gpio_test.o
	$(CC) -o am335x_gpio_test am335x_gpio_test.o

am335x_gpio_test.o : am335x_gpio_test.c
	$(CC) -c -o am335x_gpio_test.o am335x_gpio_test.c $(INC)


clean:
	rm -rf *.o
	rm am335x_gpio_test
