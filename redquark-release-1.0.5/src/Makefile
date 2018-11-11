MAINSRC     := $(shell pwd)
VICE        ?= $(MAINSRC)/vice2
FREETYPE_GL ?= $(MAINSRC)/freetype-gl

CARG := VICE=$(VICE) FREETYPE_GL=$(FREETYPE_GL) MAINSRC=$(MAINSRC)

TARGET=all

all: build

build:
	@$(MAKE) -C common   $(CARG)
	@$(MAKE) -C freetype $(CARG)
	@$(MAKE) -C viceport $(CARG)
	@$(MAKE) -C vbuild   $(CARG)
	@$(MAKE) -C input    $(CARG)
	@$(MAKE) -C ui       $(CARG) $(TARGET)

clean:
	@$(MAKE) -C common   $(CARG) clean
	@$(MAKE) -C freetype $(CARG) clean
	@$(MAKE) -C viceport $(CARG) clean
	@$(MAKE) -C vbuild   $(CARG) clean
	@$(MAKE) -C input    $(CARG) clean
	@$(MAKE) -C ui       $(CARG) clean
