QT += core
QT -= gui

TARGET = h264Streamer
CONFIG += console std-c++11 debug
CONFIG -= app_bundle
QMAKE_CXXFLAGS += -std=c++11

TEMPLATE = app

LIBS += -L/home/esoroush/local/lib -lswscale -lswresample -lavdevice -lavcodec -lavutil -lavformat -ldl -lm

LIBS += -lliveMedia -lBasicUsageEnvironment -lgroupsock -lUsageEnvironment

LIBS += /home/esoroush/libraries/x264/libx264.a
#LIBS += -L/home/esoroush/local/lib -lFFMPegVideoIO
LIBS += -ldl `pkg-config --libs opencv`

INCLUDEPATH += /home/esoroush/libraries/x264 /home/esoroush/local/include
INCLUDEPATH += /usr/local/include/groupsock
INCLUDEPATH += /usr/local/include/liveMedia
INCLUDEPATH += /usr/local/include/BasicUsageEnvironment
INCLUDEPATH += /usr/local/include/UsageEnvironment

INCLUDEPATH += ../../libsrc
LIBS += -L../../libsrc -lFFMPegMediaIO

SOURCES += \
    main.cpp
