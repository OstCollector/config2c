#!/bin/sh

err() {
	printf "%s\n" "$1"
	exit 1
}

[ -e ../config2c ] && err "../config2c existed"
[ -e ../config2c.tgz ] && err "../config2c.tar existed"
cp -r . ../config2c || err "failed to copy"
cd ../config2c || err "failed to cd to dir config2c"
make -f Makefile.prepare || err "failed to create config2cy.tab.{c,y} or config2cl.lex.c"
cd share/config2c/supplement/ || err "failed to cd to supplement"
make || err "failed to create parser.tab.{c,y} or parserl.lex.c"
cd ../../../../ || err "failed to create to base dir"
tar -c config2c | gzip > config2c.tgz || err "failed to create config2c.tgz"
