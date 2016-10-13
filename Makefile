.SUFFIXES:

DESTDIR=/usr/local

.PHONY: all clean install

all : bin/config2c

bin/config2c: config2c.c config2cy.tab.c config2cl.lex.c config2c.h config2cy.tab.h
	mkdir -p bin
	gcc -ggdb -Wenum-compare -o bin/config2c config2c.c config2cl.lex.c config2cy.tab.c

clean :
	rm config2c config2cy.tab.c config2cy.tab.h config2cl.lex.c

install :
	install -d bin ${DESTDIR}/bin
	install -m 755 -D bin/config2c ${DESTDIR}/bin/config2c
	find share/ -type d -exec install -d {} ${DESTDIR}/{} \;
	find share/ -type f -exec install -D -m 644 {} ${DESTDIR}/{} \;
