pixel.qss: pixel_screensvr.o
	gcc -shared -Wl,-soname,running_time.qss -o \
	running_time.qss running_time.o -lc

pixel_screensvr.o: pixel_screensvr.c
	gcc -g -I/usr/include/directfb -fpic -c pixel_screensvr.c

install: pixel.qss
	cp pixel.qss /etc/qingy/screensavers

