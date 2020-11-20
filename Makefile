all:
	gcc -g -Wall -Werror -fsanitize=address first.c -o first -lm

clean:
	rm -f first
