MAKEFLAGS += --no-print-directory

.PHONY: build env flash clean distclean

build:
	idf.py build

env:
# ‘useifd’ aliases ‘. /path-to/esp-idf/export.sh’
	useifd

flash:
	idf.py flash

clean:
	idf.py clean

distclean:
	idf.py fullclean
