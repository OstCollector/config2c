.SUFFIXES:

DESTDIR=/usr/local

.PHONY: all clean install cfgparser

all : bin/config2c cfgparser

bin/config2c: config2c.c config2cy.tab.c config2cl.lex.c config2c.h config2cy.tab.h
	mkdir -p bin
	gcc -ggdb -Wenum-compare -o bin/config2c config2c.c config2cl.lex.c config2cy.tab.c


config2cy.tab.c config2cy.tab.h : config2cy.y
	yacc -d -b config2cy config2cy.y

config2cl.lex.c : config2cl.l
	lex -t config2cl.l > config2cl.lex.c

cfgparser :
	$(MAKE) -C share/config2c/supplement all

clean :
	rm bin/config2c config2cy.tab.c config2cy.tab.h config2cl.lex.c
	$(MAKE) -C share/config2c/supplement clean

install :
	install -d bin ${DESTDIR}/bin
	install -m 755 -D bin/config2c ${DESTDIR}/bin/config2c
	find share/ -type d -exec install -d {} ${DESTDIR}/{} \;
	find share/ -type f -exec install -D -m 644 {} ${DESTDIR}/{} \;
