BUILD = build/
MODULES = dummy

ifeq (clean, $(findstring clean,$(MAKECMDGOALS)))
	TARGET = clean
	ACTNAME = Cleaning
else 
	TARGET = all
	ACTNAME = Building
endif

ACTIONS  = echo 						; \
		   echo $(ACTNAME) $(TARGDIR) 	; \
		   make -C $(TARGDIR) $(TARGET)

all: tsloadd run-tsload
clean: tsloadd run-tsload

build-json: 
	make -C lib/libjson

clean-json: 
	make -C lib/libjson clean

libhostinfo: 
	$(eval TARGDIR := lib/$@)
	@$(ACTIONS)

libtscommon: libhostinfo
	$(eval TARGDIR := lib/$@)
	@$(ACTIONS)

libtsload: libtscommon
	$(eval TARGDIR := lib/$@)
	@$(ACTIONS)
	
modules: $(MODULES)
	@echo
	@echo $(ACTNAME) modules...

tsloadd: libtsload modules
	$(eval TARGDIR := cmd/$@)
	@$(ACTIONS)
	
run-tsload: libtsload modules
	$(eval TARGDIR := cmd/$@)
	@$(ACTIONS)

$(MODULES):
	$(eval TARGDIR := mod/$@)
	@$(ACTIONS)
	
