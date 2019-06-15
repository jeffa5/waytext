.PHONY: build
build:
	meson build
	ninja -C build

.PHONY: install
install: build
	sudo ninja -C build install

.PHONY: run
run:
	build/waytext
