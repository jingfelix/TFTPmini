.PHONY:
all:
	@echo "compiling..."
	gcc -o bin/client client.c
	gcc -o bin/server server.c

.PHONY:
clean:
	@echo "cleaning..."
	rm -f bin/*