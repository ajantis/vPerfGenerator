BUILD = build/
MODULES = dummy

ifeq ($(MAKECMDGOALS),clean)
	TARGET = clean
	ACTION = Cleaning
else 
	TARGET = all
	ACTION = Building
endif

all: tsloadd run-tsload
clean: tsloadd run-tsload
	
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
	cp cmd/tsloadd/tsload.cfg build/
	
run-tsload: libtsload modules
	@echo
	@echo $(ACTION) run-tsload daemon...
	make -C cmd/run-tsload $(TARGET)

$(MODULES):
	@echo
	@echo $(ACTION) module $@
	make -C mod/$@ $(TARGET)
	
