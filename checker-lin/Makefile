build: so_stdio.o
	gcc -shared so_stdio.o -o libso_stdio.so

so_stdio.o:
	gcc -Wall -Wextra -fPIC -c so_stdio.c

clear:
	rm -f so_stdio.o libso_stdio.so

