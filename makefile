TARGET = zephyr
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst src/%.c, bin/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

bin/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall -o $@

clean:
	-rm -f *.out
	-rm -f *.o
	-rm -f bin/*.o
	-rm -f *.yasm
	-rm -f $(TARGET)