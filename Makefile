.PHONY: build fmt

build:
	$(MAKE) -C qmk build

fmt:
	dprint fmt
