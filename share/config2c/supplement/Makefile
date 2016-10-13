.SUFFIXES:

.PHONY: all clean

all : parsery.tab.h parsery.tab.c parserl.lex.c

parsery.tab.h parsery.tab.c : parsery.y
	bison -d -b parsery parsery.y

parserl.lex.c : parserl.l
	flex -o parserl.lex.c parserl.l

clean :
	rm parsery.tab.h parsery.tab.c parserl.lex.c

