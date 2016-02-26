#-------------------------------------------------
#
# Project created by QtCreator 2016-02-22T21:59:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ImageScissor
TEMPLATE = app




INCLUDEPATH += /usr/include
LIBS += -L/usr/lib \
    -lopencv_calib3d \
    -lopencv_contrib \
    -lopencv_core \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_gpu \
    -lopencv_highgui \
    -lopencv_imgproc


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h
