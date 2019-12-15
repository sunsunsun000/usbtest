TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        androidaccessory.cpp \
        main.cpp

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libusb-1.0

LIBS += -lpthread

HEADERS += \
    androidaccessory.h
