CC= $(TOOLCHAIN_CC)

all : ili9341_fb_test

ili9341_fb_test : ili9341_fb_test.o
	$(CC) -o ili9341_fb_test ili9341_fb_test.o

ili9341_fb_test.o : ili9341_fb_test.c
	$(CC) -c -o ili9341_fb_test.o ili9341_fb_test.c


clean:
	rm -rf *.o
	rm ili9341_fb_test