#!/bin/sh

mkdir -p make
cd make

cp ../../supplement/parser.c ./
cp ../../supplement/parser.h ./
cp ../../supplement/parsery.tab.c ./
cp ../../supplement/parsery.tab.h ./
cp ../../supplement/parserl.lex.c ./

for build in demo_0 ; do
	../../../../bin/config2c \
		--spec_path="../${build}-syntax" \
		--prim_path="../prim_funcs.c" \
		--prelude_path="../prelude" \
		--hdr_path="${build}-converter.h" \
		--src_path="${build}-converter.c" \
		--include_guard="${build}_h"
	cp "../${build}-main.c" ./
	gcc -ggdb -o "${build}" \
		parser.c \
		parsery.tab.c \
		parserl.lex.c \
		"${build}-main.c" \
		"${build}-converter.c"
	
	../../../../bin/config2c \
		--spec_path="../${build}-syntax" \
		--prim_path="../prim_funcs.c" \
		--prelude_path="../prelude" \
		--hdr_path="${build}-test.h" \
		--src_path="${build}-test.c" \
		--include_guard="${build}_h" \
		--test_default
	gcc -ggdb -o "${build}-default-test" \
		parser.c \
		parsery.tab.c \
		parserl.lex.c \
		"${build}-test.c"
done
