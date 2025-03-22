tergen: Makefile tergen.c
	gcc  -march=native -g -O2 -o tergen tergen.c -lm
