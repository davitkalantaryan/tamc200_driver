#
# file:		Makefile 
# purpose:	Makefile for compiling Linux device driver for tamc200 devices
# 
# export KBUILD_EXTRA_SYMBOLS=~/dev/.desy/tamc200_driver_ide/contrib/upciedev/Upciedev.symvers
# or
# make -C src -e KBUILD_EXTRA_SYMBOLS=$PWD/../contrib/upciedev/Upciedev.symvers
# or
# make -C src KBUILD_EXTRA_SYMBOLS=$PWD/../../contrib/upciedev/Upciedev.symvers
#

MODULE_NAME	= tamc200
mkfile_path	= $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir	= $(shell dirname $(mkfile_path))
repositoryRoot	= $(mkfile_dir)/..

KVERSION	= $(shell uname -r)
HOSTNAME	= $(shell hostname)

ifndef KO_FILES
	KO_FILES = $(repositoryRoot)/ko_files/$(HOSTNAME)_$(KVERSION)
endif

ifndef UPCIEDEV_INCLUDE
	UPCIEDEV_INCLUDE = -I$(mkfile_dir)/../../contrib/upciedev -I/usr/include -I/usr/src/upciedev-1.0.105
endif

CUR_DIR		=$(mkfile_dir)

$(MODULE_NAME)-objs := \
entry_tamc200_driver.o	\
pciedev_ufn2.o
	
obj-m := $(MODULE_NAME).o

#all: compile install
all: compile
default: compile

copy:
	# sudo cp $(KO_FILES)/$(MODULE_NAME).ko /lib/modules/$(KVERSION)/desy_zeuthen/.
install:
	#sudo rmmod timer_drv
	#sudo insmod timer_drv.ko
	#sudo cp timer_drv.ko /lib/modules/$(KVERSION)/desy_zeuthen/.
compinstall:
	#cp ../../pcie_gen/pcie_gen_mkfl/Module.symvers.$(HOSTNAME2).$(KVERSION) ./Module.symvers
	#cp $(SOURCES_DIR)/*.c .
	#cp $(SOURCES_DIR)/*.h .
	#make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	#rm -f *.c
	#rm -f *.h
	#-sudo rmmod pcie_gen_drv
	#sudo insmod pcie_gen_drv.ko
	#sudo chmod 666 /dev/pcie_gen_init
compile:
	#cp $(KO_FILES)/Module.symvers.pcie_gen_drv ./Module.symvers
	make -C /lib/modules/$(KVERSION)/build M=$(mkfile_dir) modules
	mkdir -p $(KO_FILES)
	cp Module.symvers $(KO_FILES)/Module.symvers.$(MODULE_NAME)
	cp $(MODULE_NAME).ko $(KO_FILES)/.
.PHONY: clean
clean:
	rm -f $(KO_FILES)/$(MODULE_NAME).ko
	rm -f $(KO_FILES)/Module.symvers.$(MODULE_NAME)
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build M=$(mkfile_dir) clean
	

EXTRA_CFLAGS	+= $(UPCIEDEV_INCLUDE)
#EXTRA_CFLAGS	+= -D_NEW_ADDED_CODE -DUSE_SEMAPHORE
ifndef KBUILD_EXTRA_SYMBOLS
	KBUILD_EXTRA_SYMBOLS = /lib/modules/$(KVERSION)/upciedev/Upciedev.symvers
endif
