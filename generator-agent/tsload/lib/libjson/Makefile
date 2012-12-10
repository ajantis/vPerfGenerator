TYPE = SHARED
TARGET = libjson.so

include ../../Makefile.master

SRC        =     internalJSONNode.cpp JSONAllocator.cpp JSONChildren.cpp \
                 JSONDebug.cpp JSONIterators.cpp JSONMemory.cpp JSONNode.cpp \
                 JSONNode_Mutex.cpp JSONPreparse.cpp JSONStream.cpp JSONValidator.cpp \
                 JSONWorker.cpp JSONWriter.cpp libjson.cpp 
OBJ = $(SRC:.cpp=.o)

CXXFLAGS += -Os -ffast-math -DJSON_LESS_MEMORY
LDFLAGS += -lstdc++
	
include ../../Makefile.build