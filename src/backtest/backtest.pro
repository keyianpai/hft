TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11


HEADERS += \
        strategy.h \
        order_handler.h \

SOURCES += \
        main.cpp \
        strategy.cpp \
        order_handler.cpp \

INCLUDEPATH += $$PWD/../../external/common/include
INCLUDEPATH += $$PWD/../../external/ctp/include
INCLUDEPATH += $$PWD/../../external/zmq/include
INCLUDEPATH += $$PWD/../../external/libconfig/include
INCLUDEPATH += /root/anaconda2/include/python2.7
INCLUDEPATH += $$PWD/..


LIBS += -L$$PWD/lib64 -lpthread
LIBS += -L$$PWD/../../external/common/lib -lpython2.7
LIBS += -L$$PWD/../../external/zmq/lib -lzmq

LIBS += -L$$PWD/../../external/common/lib -lcommontools
LIBS += -L$$PWD/../../external/libconfig/lib -lconfig++
LIBS += -L$$PWD/../../external/ctp/lib -lthosttraderapi

