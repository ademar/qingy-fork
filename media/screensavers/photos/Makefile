photos.qss: photos_screensvr.o
	gcc -shared -Wl,-soname,photos.qss -o \
	photos.qss photos_screensvr.o -lc

photos_screensvr.o: photos_screensvr.c
	gcc -g -I/usr/include/directfb -I/etc/qingy/screensavers -fpic -c photos_screensvr.c

install: photos.qss
	cp photos.qss /etc/qingy/screensavers

clean:
	rm photos.qss
	rm photos_screensvr.o
