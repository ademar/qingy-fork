BINDIR      = /bin
OBJS        = src/misc.o src/chvt.o src/directfb_utils.o src/framebuffer_mode.o src/session.o src/main.o

#What is the C compiler?
ifndef CC
	CC = gcc
endif

#Is it gcc?
ifeq "$(findstring gcc,$(shell $(CC) --version))" "gcc"
  #Is it's version >= 3?
	ifeq "$(findstring 3.,$(shell $(CC) -dumpversion))" "3."
	  #We can set processor specific optimizations
  	PROCESSOR=$(shell cat /proc/cpuinfo)
		#Is our CPU a Celeron?
	  ifeq "$(findstring Celeron,$(PROCESSOR))" "Celeron"
		  CPUFLAG = -march=pentium2
		endif
		ifeq "$(findstring Celeron (Coppermine),$(PROCESSOR))" "Celeron (Coppermine)"
	 		CPUFLAG = -march=pentium3
		endif
		#Is our CPU a Pentium?
		ifeq "$(findstring Pentium,$(PROCESSOR))" "Pentium"
			CPUFLAG = -march=pentium
		endif
		#Is our CPU a Pentium II?
		ifeq "$(findstring Pentium II,$(PROCESSOR))" "Pentium II"
			CPUFLAG = -march=pentium2
		endif
		#Is our CPU a Pentium III?
		ifeq "$(findstring Pentium III,$(PROCESSOR))" "Pentium III"
			CPUFLAG = -march=pentium3
		endif
		ifeq "$(findstring Intel(R) Pentium(R) 4,$(PROCESSOR))" "Intel(R) Pentium(R) 4"
			CPUFLAG = -march=pentium4
		endif
		ifeq "$(findstring AMD Athlon(tm),$(PROCESSOR))" "AMD Athlon(tm)"
		  CPUFLAG = -march=athlon
		endif
		ifeq "$(findstring AMD Athlon(tm) MP,$(PROCESSOR))" "AMD Athlon(tm) MP"
		  CPUFLAG = -march=athlon-mp
		endif
		ifeq "$(findstring AMD Athlon(tm) XP,$(PROCESSOR))" "AMD Athlon(tm) XP"
		  CPUFLAG = -march=athlon-xp
		endif
		#Is our CPU an Ultra Sparc?
		ifeq "$(findstring UltraSparc,$(PROCESSOR))" "UltraSparc"
			CPUFLAG = -mcpu=ultrasparc
		endif
	endif
	ifndef PROCESSOR
	  #Alas, we can set only generic arch optimizations
		PROCESSOR=$(shell uname -m)
		#Is our CPU an x86?
		ifeq "$(findstring 86,$(PROCESSOR))" "86"
			CPUFLAG = -march=$(PROCESSOR)
		endif
	endif
endif
ifndef CFLAGS
  CFLAGS = $(CPUFLAG) -O3 -pipe -fforce-addr -W -Wall -pedantic
else
  CFLAGS = $(CFLAGS) -W -Wall -pedantic
endif
DFBCFLAGS = $(shell echo `pkg-config --cflags directfb`)
DFBLIBS = $(shell echo `pkg-config --libs directfb`)

.c.o:
	$(CC) -D$(shell echo `uname -s`) $(CFLAGS) $(DFBCFLAGS) -c $< -o $*.o

all: qingy

qingy:	$(OBJS)
	$(CC) $(CFLAGS) $(DFBLIBS) -o qingy $^

clean:
	for i in $(OBJS) ; do \
		rm -f $$i; \
	done
	rm -f qingy
	rm -f *~

install::	qingy
	install -s -m500 qingy $(PREFIX)$(BINDIR)
uninstall::	qingy
	-rm -f $(PREFIX)$(BINDIR)/qingy
	@echo
	@echo Program uninstalled.
