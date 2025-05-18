#-------------------------------------------------
#
# Project created by QtCreator 2013-04-05T19:35:15
#
#-------------------------------------------------

QT       += core gui serialport widgets network

TARGET = ThreeButtonPublisherSubscriberJSON
TEMPLATE = app


SOURCES += main.cpp\
        guipanel.cpp

HEADERS  += guipanel.h

FORMS    += guipanel.ui

CONFIG   +=qwt # Añadir para usar Qwidgets

QT += widgets #Para QT4-->QT5

CONFIG +=qwt analogwidgets # Añadir para usar Qwidgets

CONFIG += qwt qmqtt

target.path    = "C:\MyApp"
INSTALLS       = target
