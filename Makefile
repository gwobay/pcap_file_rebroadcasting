MAKERULES=makerules
include $(MAKERULES)/streamline.mk

.PHONY: all

PROJECTS = tools core md cwtest

all:    bin
	@for proj in $(PROJECTS); do  if [ -d $${proj} ]; then cd $${proj} && $(MAKE) || exit 2 && cd ..; fi; done;

bin:
	mkdir -p bin

clean:
	rm -rf bin/*
	rm -rf lib/*
	@for proj in $(PROJECTS); do if [ -d $${proj} ]; then cd $${proj} && $(MAKE) clean && cd ..; fi; done;

cleandeps:
	find . -name "*.d" | xargs rm -f 

