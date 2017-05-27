dcclient: *.c
	gcc -g $^ -o $@ -lm -lpthread
