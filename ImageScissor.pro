#-------------------------------------------------
#
# Project created by QtCreator 2016-02-22T21:59:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Test
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

INCLUDEPATH += /usr/local/Cellar/opencv/2.4.12/include
LIBS += -L/usr/local/Cellar/opencv/2.4.12/lib \
    -lopencv_calib3d \
    -lopencv_contrib \
    -lopencv_core \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_gpu \
    -lopencv_highgui \
    -lopencv_imgproc \
    -lopencv_legacy \
    -lopencv_ml \
    -lopencv_nonfree \
    -lopencv_objdetect \
    -lopencv_ocl \
    -lopencv_photo \
    -lopencv_stitching \
    -lopencv_superres \
    -lopencv_ts \
    -lopencv_video

