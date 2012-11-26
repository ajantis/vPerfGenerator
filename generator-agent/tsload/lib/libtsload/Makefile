TYPE = SHARED
TARGET = libtsload.so

include ../../Makefile.master

SRC = threadpool.c \
	  wlparam.c \
	  workload.c \
	  modtsload.c
	  
OBJ = $(SRC:.c=.o)

JSON_DEBUG = -DNDEBUG

CFLAGS += $(INCJSON) $(INCTSCOMMON) $(JSON_DEBUG)
LDFLAGS += -lc -ldl $(LINKJSON) $(LINKTSCOMMON)
	
include ../../Makefile.build