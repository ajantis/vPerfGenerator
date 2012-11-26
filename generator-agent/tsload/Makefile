BUILD = build/
MODULES = dummy

ifeq (clean, $(findstring clean,$(MAKECMDGOALS)))
	TARGET = clean
	ACTION = Cleaning
else 
	TARGET = all
	ACTION = Building
endif

all: tsloadd run-tsload
clean: tsloadd run-tsload

build-json: libjson
clean-json: libjson
	
libjson: 
	@echo
	@echo $(ACTION) libjson...
	make -C lib/libjson $(TARGET)
	
libtscommon: 
	@echo
	@echo $(ACTION) libtscommon...
	make -C lib/libtscommon $(TARGET)

libtsload: libtscommon
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
	
run-tsload: libtsload modules
	@echo
	@echo $(ACTION) run-tsload daemon...
	make -C cmd/run-tsload $(TARGET)

$(MODULES):
	@echo
	@echo $(ACTION) module $@
	make -C mod/$@ $(TARGET)
	
