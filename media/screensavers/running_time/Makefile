running_time.qss: running_time.o
	gcc -shared -Wl,-soname,running_time.qss -o \
	running_time.qss running_time.o -lc

running_time.o: running_time.c
	gcc -g -I/usr/include/directfb -I/etc/qingy/screensavers -fpic -c running_time.c

install: running_time.qss
	cp running_time.qss /etc/qingy/screensavers

clean:
	rm running_time.qss
	rm running_time.o
