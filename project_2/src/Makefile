.DEFAULT_GOAL := rcom

COMPILER_TYPE= gnu
CC = gcc
CCFLAGS= -Wall

PROG = rcom.out

SOURCES = URL.c ftp.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY : rcom
rcom : $(OBJECTS)
	$(CC) $(CCFLAGS) main.c $(OBJECTS) -o $(PROG)
	rm -f $(OBJECTS)

# To obtain object files
%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

# To remove generated files
.PHONY: clean
clean:
	rm -f $(PROG) $(OBJECTS)
