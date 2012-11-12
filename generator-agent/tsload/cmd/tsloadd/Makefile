TYPE = EXEC
TARGET = tsloadd

include ../../Makefile.master

SRC = main.c 
OBJ = $(SRC:.c=.o)

CFLAGS += -I../../lib/libtsload/include
LDFLAGS += -ltsload -lc -ljson
	
include ../../Makefile.build