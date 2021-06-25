
# 

TEMPLATE = aux

CONFIG -= app_bundle
CONFIG -= qt

DEFINES += __INTELISENSE__

ARCH=arm64
SRC_PROJECT_PATH = $${PWD}/../../src
KERNEL_VERSION = $$system(uname -r)
LINUX_HEADERS_PATH = /usr/src/linux-headers-$${KERNEL_VERSION}

#SOURCES += $$system(find -L $$SRC_PROJECT_PATH -type f -name "*.c" -o -name "*.S" )
HEADERS += $$system(find -L $$SRC_PROJECT_PATH -type f -name "*.h" )
OTHER_FILES += $$system(find -L $$SRC_PROJECT_PATH -type f -not -name "*.h" -not -name "*.c" -not -name "*.S" )

INCLUDEPATH += $${LINUX_HEADERS_PATH}/include
INCLUDEPATH += /usr/src/linux-headers-5.4.0-73/include
INCLUDEPATH += $$system(find -L $$SRC_PROJECT_PATH -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/arch/$$ARCH/include -type d)


SOURCES +=	\
        $${SRC_PROJECT_PATH}/entry_tamc200_driver.c
