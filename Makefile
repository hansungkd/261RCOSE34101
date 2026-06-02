CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
TARGET := scheduler
OBJS := main.o process.o queue.o gantt.o scheduler.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
