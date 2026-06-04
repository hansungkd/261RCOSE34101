CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
TARGET := scheduler
OBJS := main.o process.o queue.o priority_queue.o gantt.o scheduler.o \
	scheduler_fcfs.o scheduler_sjf.o scheduler_priority.o scheduler_rr.o

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
