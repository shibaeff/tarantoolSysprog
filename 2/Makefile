CC	=  cc
CFLAGS	=  -I .
CFLAGS	+= -W -Wall -Wextra -Werror -Wfloat-equal
CFLAGS	+= -Wundef -Wpointer-arith -Wcast-align -Wshadow
CFLAGS	+= -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return
CFLAGS	+= -Wswitch-enum -Wunreachable-code -Winit-self
CFLAGS	+= -Wno-unused-parameter -pedantic -O3
LDFLAGS	=

BASE_SOURCES    = main.c
SOURCES		= $(BASE_SOURCES)
OBJS		= $(SOURCES:.c=.o)
EXECUTABLE	= task_2

all: test

build:
	gcc $(CFLAGS)  $(LDLAGS)   main.c

test: build
	python3 checker.py -e ./a.out --max=15

clean:
	rm -rf ./a.out $(OBJS)

.PHONY: clean, build
