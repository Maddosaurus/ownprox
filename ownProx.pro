#-------------------------------------------------
#
# Project created by QtCreator 2013-11-23T15:07:42
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = ownProx
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   -= qt

TEMPLATE = app


SOURCES += main.cpp



INCLUDEPATH += /home/shivy/Downloads/live/liveMedia/include     \
    /home/shivy/Downloads/live/BasicUsageEnvironment/include    \
    /home/shivy/Downloads/live/groupsock/include                \
    /home/shivy/Downloads/liveMedia/UsageEnvironment/include


LIBS += /usr/local/lib/libliveMedia.a \
        /usr/local/lib/libBasicUsageEnvironment.a \
        /usr/local/lib/libUsageEnvironment.a \
        /usr/local/lib/libgroupsock.a
