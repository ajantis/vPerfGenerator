TYPE = EXEC
TARGET = tsloadd

include ../../Makefile.master

SRC = main.c \
	  cfgfile.c 
	  
OBJ = $(SRC:.c=.o)
EXTRAS = tsload.cfg

CFLAGS += $(INCTSLOAD) $(INCJSON)
LDFLAGS += -lc $(LINKTSLOAD) $(LINKJSON)
	
include ../../Makefile.build