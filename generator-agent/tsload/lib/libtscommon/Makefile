TYPE = SHARED
TARGET = libtscommon.so

include ../../Makefile.master

SRC = hashmap.c \
	  log.c \
	  threads.c \
	  tutil.c	\
	  client.c	\
	  modules.c \
	  syncqueue.c \
	  mempool.c
	  
OBJ = $(SRC:.c=.o)

CFLAGS += $(INCJSON) $(JSON_DEBUG)
LDFLAGS += -lc -ldl -lpthread $(LINKJSON)
	
include ../../Makefile.build