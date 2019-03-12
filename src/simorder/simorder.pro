TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
        listener.h \
        orderlistener.h \

SOURCES += \
        main.cpp \
        orderlistener.cpp \

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
