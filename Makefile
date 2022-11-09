CC=gcc
SRC=src
TARGET=bin

.PHONY:
build:
	@echo "compiling..."
	$(CC) -Os -Wl,--gc-sections -o $(TARGET)/client $(SRC)/client.c $(SRC)/tftp.c
	$(CC) -Os -Wl,--gc-sections -o $(TARGET)/server $(SRC)/server.c $(SRC)/tftp.c

.PHONY:
clean:
	@echo "cleaning..."
	rm -f $(TARGET)/*

.PHONY:
debug:
	@echo "compiling debug target"
	$(CC) -g -D DEBUG -o $(TARGET)/client $(SRC)/client.c $(SRC)/tftp.c
	$(CC) -g -D DEBUG -o $(TARGET)/server $(SRC)/server.c $(SRC)/tftp.c
	