
# 

upciedevRepoRoot = $${PWD}/../../../contrib/upciedev_ide
include ("$${upciedevRepoRoot}/workspaces/upciedev_ide_qt/idrivers.pri")
repositoryRoot = $${PWD}/../..

TEMPLATE = aux

CONFIG -= app_bundle
CONFIG -= qt

DEFINES += __INTELISENSE__
DEFINES += __KERNEL__

DOCS_TXT	= $$system($${upciedevRepoRoot}/scripts/findfiles $${repositoryRoot}/docs   .txt)
DOCS_MD		= $$system($${upciedevRepoRoot}/scripts/findfiles $${repositoryRoot}/docs   .md)
DEBIAN_ALL	= $$system($${upciedevRepoRoot}/scripts/findfiles $${repositoryRoot}/debian   )

ARCH=arm64
repositoryRoot = $${PWD}/../..
SRC_PROJECT_PATH = $${repositoryRoot}/src
KERNEL_VERSION = $$system(uname -r)
LINUX_HEADERS_PATH = /usr/src/linux-headers-$${KERNEL_VERSION}

#SOURCES += $$system(find -L $$SRC_PROJECT_PATH -type f -name "*.c" -o -name "*.S" )
#HEADERS += $$system(find -L $$SRC_PROJECT_PATH -type f -name "*.h" )
OTHER_FILES += $$system(find -L $$SRC_PROJECT_PATH -type f -not -name "*.h" -not -name "*.c" -not -name "*.S" )

# here add all developer host specific include paths
INCLUDEPATH += /usr/src/linux-headers-5.4.0-73/include

INCLUDEPATH += "$${repositoryRoot}/../contrib/upciedev"
INCLUDEPATH += "$${LINUX_HEADERS_PATH}/include"
INCLUDEPATH += $$system(find -L $$SRC_PROJECT_PATH -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/arch/$$ARCH/include -type d)


SOURCES +=	\
	"$${SRC_PROJECT_PATH}/entry_tamc200_driver.c"			\
	"$${SRC_PROJECT_PATH}/pciedev_ufn2.c"					\
	\
	"$${repositoryRoot}/tests/main_hotplug_test.cpp"		\
	"$${repositoryRoot}/tests/main_mmap_test.cpp"

HEADERS +=	\
        "$${SRC_PROJECT_PATH}/pciedev_ufn2.h"				\
	"$${SRC_PROJECT_PATH}/tamc200_io.h"

OTHER_FILES += $${DOCS_TXT}
OTHER_FILES += $${DOCS_MD}
OTHER_FILES += $${DEBIAN_ALL}


OTHER_FILES +=	\
        "$${repositoryRoot}/.gitattributes"		\
	"$${repositoryRoot}/.gitignore"			\
	"$${repositoryRoot}/dkms.conf"			\
	"$${repositoryRoot}/LICENSE"			\
	"$${repositoryRoot}/README.md"			\
	"$${repositoryRoot}/src/Makefile"			\
	"$${repositoryRoot}/tests/hotplug_test.Makefile"	\
	"$${repositoryRoot}/tests/mmap_test.Makefile"
