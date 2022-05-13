ww: ww.c
	gcc -Wall -pthread -g -fsanitize=address -o ww ww.c