#-------------------------------------------------
#
# Project created by QtCreator 2014-01-30T14:08:10
#
#-------------------------------------------------

QT       += core gui testlib network

TARGET = AVT_Camera_Controller
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += _LINUX _arm

SOURCES += main.cpp \
    main_controller.cpp


INCLUDEPATH += "/home/sunny/Desktop/AVT GigE SDK/inc-pc"
LIBS += "/home/sunny/Desktop/AVT GigE SDK/lib-pc/arm/4.4/libPvAPI.a" "/usr/arm-linux-gnueabi/lib/librt.so"

HEADERS += \
    main_controller.h
