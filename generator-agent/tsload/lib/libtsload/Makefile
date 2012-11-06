TYPE = SHARED
TARGET = lib/libtsload.so

include ../../Makefile.master

SRC = log.c \
	  modules.c
	  
OBJ = $(SRC:.c=.o)

LDFLAGS += -lc -ldl
	
include ../../Makefile.build