#-------------------------------------------------
#
# Project created by QtCreator 2015-05-23T15:22:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = patchmatch
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    imageview.cpp \
    zone.cpp \
    patchmatchalgo.cpp \
    progressdialog.cpp \
    settingsdialog.cpp \
    retargetdialog.cpp

HEADERS  += mainwindow.h \
    imageview.h \
    zone.h \
    patchmatchalgo.h \
    progressdialog.h \
    settingsdialog.h \
    retargetdialog.h

FORMS    += mainwindow.ui \
    progressdialog.ui \
    settingsdialog.ui \
    retargetdialog.ui

RESOURCES += \
    icons.qrc
