pixel.qss: pixel_screensvr.o
	gcc -shared -Wl,-soname,pixel.qss -o \
	pixel.qss pixel_screensvr.o -lc

pixel_screensvr.o: pixel_screensvr.c
	gcc -g -I/usr/include/directfb -I/etc/qingy/screensavers -fpic -c pixel_screensvr.c

install: pixel.qss
	cp pixel.qss /etc/qingy/screensavers

clean:
	rm pixel.qss
	rm pixel_screensvr.o
