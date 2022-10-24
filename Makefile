.PHONY:
all:
	@echo "compiling..."
	gcc -g -o bin/client client.c
	gcc -g -o bin/server server.c

.PHONY:
clean:
	@echo "cleaning..."
	rm -f bin/*