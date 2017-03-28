#-------------------------------------------------
#
# Project created by QtCreator 2017-03-24T20:53:40
#
#-------------------------------------------------

QT       -= core gui

HOME = /home/esoroush

TARGET = FFMPegVideoIO
TEMPLATE = lib

DEFINES += FFMPEGVIDEOIO_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
header_files.files = ../$$TARGET/*.h
header_files.path = $$HOME/local/include/lib$$TARGET
INSTALLS += header_files



# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    muxer.cpp \
    avMediaDecoder.cpp \
    clsVideoFramedSource.cpp \
    clsLiveServerMediaSubsession.cpp \
    x264encoder.cpp \
    demuxer.cpp \
    hlpFunctions.cpp

HEADERS += \
    clsVideoFramedSource.h \
    clsLiveServerMediaSubsession.h \
    muxer.h \
    x264encoder.h \
    avMediaDecoder.h \
    demuxer.h \
    hlpFunctions.h

LIBS += -L$$HOME/local/lib -lswscale -lswresample -lavdevice -lavcodec -lavutil -lavformat -ldl -lm

LIBS += -lliveMedia -lBasicUsageEnvironment -lgroupsock -lUsageEnvironment

LIBS += -L$$HOME/libraries/x264/libx264.so
LIBS += -ldl `pkg-config --libs opencv`

INCLUDEPATH += $$HOME/libraries/x264 $$HOME/local/include
INCLUDEPATH += /usr/local/include/groupsock
INCLUDEPATH += /usr/local/include/liveMedia
INCLUDEPATH += /usr/local/include/BasicUsageEnvironment
INCLUDEPATH += /usr/local/include/UsageEnvironment



unix {
    target.path = $$HOME/local/lib
    INSTALLS += target
}
