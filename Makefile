PROJECT_NAME := cot #target file name

CC := gcc #compiler
CFLAGS := -g

FILES := $(wildcard src/*.c)
HEADERS := $(wildcard headers/*.h)
OBJECTS := $(addprefix bin/,$(notdir $(FILES:.c=.o)))

VPATH := src

all: bin $(PROJECT_NAME)

cot: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROJECT_NAME) $(OBJECTS)

bin/%.o: %.c | $(HEADERS)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

bin:
	mkdir -p bin

clean:
	rm -rf bin/* $(PROJECT_NAME)