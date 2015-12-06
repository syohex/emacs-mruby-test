EMACS_ROOT ?= ../..
EMACS ?= emacs

CC      = gcc
LD      = gcc
CPPFLAGS = -I$(EMACS_ROOT)/src -Imruby/include
CFLAGS = -std=gnu99 -ggdb3 -Wall -fPIC $(CPPFLAGS)
LDFLAGS = -L mruby/build/host/lib -lmruby

.PHONY : test

all: mruby-core.so

mruby-core.so: mruby-core.o mruby/build/host/lib/libmruby.a
	$(LD) -shared $(LDFLAGS) -o $@ $^

mruby-core.o: mruby-core.c
	$(CC) $(CFLAGS) -c -o $@ $<

mruby/build/host/lib/libmruby.a:
	(cd mruby && make CFLAGS="-std=gnu99 -g -fPIC")

clean:
	-rm -f mruby-core.so mruby-core.o
	-(cd mruby && make clean)

test:
	$(EMACS) -Q -batch -L . $(LOADPATH) \
		-l test/test.el \
		-f ert-run-tests-batch-and-exit
