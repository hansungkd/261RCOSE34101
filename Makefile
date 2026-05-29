CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
TARGET := scheduler

.PHONY: all clean run

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
