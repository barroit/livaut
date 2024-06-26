MAKEFLAGS += --no-print-directory

.PHONY: build flash clean distclean monitor

build:
	idf.py build

flash:
	idf.py flash

clean:
	idf.py clean

distclean:
	idf.py fullclean

monitor:
	idf.py monitor
