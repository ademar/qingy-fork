TARGET=$(notdir $(PWD)).qss
OBJECTS=$(patsubst %.c,%.o,$(wildcard *.c))
SOURCES=$(wildcard *.c)
AUX=Makefile COPYING
INSTALLDIR=/etc/qingy/screensavers
CFLAGS=-DHAVE_DIRENT_H -g -I/usr/include/directfb -I/etc/qingy/screensavers -fpic 

all: $(TARGET)

$(TARGET): $(OBJECTS)
	gcc -shared -Wl,-soname,$(TARGET) -o $(TARGET) $(OBJECTS) -lc

.PHONY: install clean dist

install: $(TARGET)
	@cp $(TARGET) $(INSTALLDIR)
	@echo "$(TARGET) installed in $(INSTALLDIR)..."
clean:
	@rm $(TARGET) $(OBJECTS)

dist: 
	@mkdir $(notdir $(PWD))
	@ln $(SOURCES) $(AUX) $(notdir $(PWD))
	@echo "Making dist: $(notdir $(PWD)).tar.gz"
	@tar -chzf $(notdir $(PWD)).tar.gz $(notdir $(PWD))
	@rm -rf $(notdir $(PWD))
	@echo "dist $(notdir $(PWD)).tar.gz made"

