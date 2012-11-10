TYPE = SHARED
TARGET = lib/libtsload.so

include ../../Makefile.master

SRC = log.c \
	  modules.c \
	  threadpool.c \
	  threads.c
	  
OBJ = $(SRC:.c=.o)

LDFLAGS += -lc -ldl -lpthread
	
include ../../Makefile.build