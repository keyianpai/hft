TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
        order_handler.h \

SOURCES += \
        main.cpp \
        order_handler.cpp \

INCLUDEPATH += $$PWD/../../external/common/include
INCLUDEPATH += $$PWD/../../external/ctp/include
INCLUDEPATH += $$PWD/../../external/zmq/include
INCLUDEPATH += $$PWD/../../external/libconfig/include
INCLUDEPATH += $$PWD/..

LIBS += -L$$PWD/../../external/zmq/lib -lzmq
LIBS += -L$$PWD/lib64 -lpthread

LIBS += -L$$PWD/../../external/common/lib -lcommontools
LIBS += -L$$PWD/../../external/libconfig/lib -lconfig++
LIBS += -L$$PWD/../../external/ctp/lib -lthosttraderapi
