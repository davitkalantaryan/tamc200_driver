
# to compile:  make -f mmap_test.Makefile UPCIEDEV_INCLUDE=${PWD}/../../contrib/upciedev

TARGET_NAME     = mmap_test

mkfile_path     = $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir      = $(shell dirname $(mkfile_path))

firstTarget: $(TARGET_NAME)

ifndef UPCIEDEV_INCLUDE
	UPCIEDEV_INCLUDE = /usr/include
endif

CPPFLAGS = -I$(UPCIEDEV_INCLUDE)

main_mmap_test.o: main_mmap_test.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

$(TARGET_NAME): main_mmap_test.o
	g++ $^ $(LIBS) $(LFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(TARGET_NAME)

