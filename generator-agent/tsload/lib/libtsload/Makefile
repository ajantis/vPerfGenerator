TYPE = SHARED
TARGET = lib/libtsload.so

include ../../Makefile.master

SRC = log.c \
	  modules.c \
	  threadpool.c \
	  threads.c \
	  wlparam.c \
	  workload.c
	  
OBJ = $(SRC:.c=.o)

JSON_DEBUG = -DNDEBUG

CFLAGS += $(INCJSON) $(JSON_DEBUG)
LDFLAGS += -lc -ldl -lpthread $(LINKJSON)
	
include ../../Makefile.build