CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
TARGET := scheduler
OBJS := main.o process.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
