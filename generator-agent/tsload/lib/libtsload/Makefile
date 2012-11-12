TYPE = SHARED
TARGET = lib/libtsload.so

include ../../Makefile.master

SRC = log.c \
	  modules.c \
	  threadpool.c \
	  threads.c \
	  wlparam.c
	  
OBJ = $(SRC:.c=.o)

JSON_DEBUG = -DNDEBUG

CFLAGS += -I../libjson/include/ $(JSON_DEBUG)
LDFLAGS += -lc -ldl -lpthread -ljson
	
include ../../Makefile.build