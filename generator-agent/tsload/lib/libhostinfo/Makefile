TYPE = SHARED
TARGET = libhostinfo.so

include ../../Makefile.master

SRC = plat/posix/uname.c

OBJSUBDIRS += plat/posix/ \
			  plat/linux/

OBJ = $(SRC:.c=.o)

CFLAGS += $(INCJSON) $(INCTSCOMMON) $(JSON_DEBUG)
LDFLAGS += -lc -ldl -lpthread $(LINKJSON)
	
include ../../Makefile.build