
#apt install libfreetype-dev
#CFLAGS = -I/usr/local/include/freetype2

#CFLAGS = -I/usr/local/include/freetype2
CFLAGS = -I/usr/include/freetype2
LINKS = -L/usr/local/lib -lfreetype
all: Makefile fontprocessor.c
	$(CC) fontprocessor.c -o renderer $(CFLAGS) $(LINKS)
clean:
	rm -rf renderer
	rm -rf dest_dir

debug:
	$(CC) -g fontprocessor.c -o renderer $(CFLAGS) $(LINKS)
