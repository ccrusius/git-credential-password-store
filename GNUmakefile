PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
MANDIR ?= $(PREFIX)/share/man
PROG = git-credential-password-store

all: $(PROG)

$(PROG): $(PROG).c
	$(CC) -std=c99 -o $@ $<
	strip $@

test: $(PROG)
	echo "host=github.com\n" | ./$(PROG) get
	echo "host=github.com\nusername=notme\n" | ./$(PROG) get
	echo "host=github.com\nusername=cesar.crusius@stanfordalumni.org\n" | ./$(PROG) get

install:
	@install -v -d "$(DESTDIR)$(BINDIR)"
	@install -m 0755 -v $(PROG) "$(DESTDIR)$(BINDIR)/$(PROG)"


uninstall:
	@rm -vrf \
		"$(DESTDIR)$(BINDIR)/$(PROG)"
