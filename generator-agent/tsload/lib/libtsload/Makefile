TYPE = SHARED
TARGET = libtsload.so

include ../../Makefile.master

SRC = threadpool.c \
	  wlparam.c \
	  workload.c \
	  modtsload.c \
	  loadagent.c \
	  tsload.c \
	  worker.c \
	  disp/simple.c 
	  
OBJ = $(SRC:.c=.o)
OBJSUBDIRS += disp/

JSON_DEBUG = -DNDEBUG

CFLAGS += $(INCJSON) $(INCTSCOMMON) $(JSON_DEBUG)
LDFLAGS += -lc -ldl $(LINKJSON) $(LINKTSCOMMON)
	
include ../../Makefile.build