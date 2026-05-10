all:
	gcc main.c -o main

clean:
	rm -rf main

run:
	gcc main.c -o main && ./main
