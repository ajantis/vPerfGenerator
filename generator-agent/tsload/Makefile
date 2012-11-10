BUILD = build/
MODULES = dummy

ifeq ($(MAKECMDGOALS),clean)
	TARGET = clean
	ACTION = Cleaning
else 
	TARGET = all
	ACTION = Building
endif

all: libtsload tsloadd modules 

clean: modules tsloadd libtsload 
	

libtsload:
	@echo
	@echo $(ACTION) libtsload...
	make -C lib/libtsload $(TARGET)

tsloadd:
	@echo
	@echo $(ACTION) tsload daemon...
	make -C cmd/tsloadd $(TARGET)
	
modules: $(MODULES)
	@echo
	@echo $(ACTION) modules...

$(MODULES):
	@echo
	@echo $(ACTION) module $@
	make -C mod/$@ $(TARGET)
	
