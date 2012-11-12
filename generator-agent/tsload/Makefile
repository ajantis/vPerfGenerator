BUILD = build/
MODULES = dummy

ifeq ($(MAKECMDGOALS),clean)
	TARGET = clean
	ACTION = Cleaning
else 
	TARGET = all
	ACTION = Building
endif

all: tsloadd
clean: tsloadd
	
libjson: 
	@echo
	@echo $(ACTION) libjson...
	make -C lib/libjson $(TARGET)

libtsload: libjson
	@echo
	@echo $(ACTION) libtsload...
	make -C lib/libtsload $(TARGET)
	
modules: $(MODULES)
	@echo
	@echo $(ACTION) modules...

tsloadd: libtsload modules
	@echo
	@echo $(ACTION) tsload daemon...
	make -C cmd/tsloadd $(TARGET)

$(MODULES):
	@echo
	@echo $(ACTION) module $@
	make -C mod/$@ $(TARGET)
	
