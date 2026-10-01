QT -= gui
QT += core
QT += concurrent

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = ConsoleCalculator
TEMPLATE = app

SOURCES += \
    main.cpp \
    calculator.cpp \
    consolereader.cpp

HEADERS += \
    calculator.h \
    consolereader.h
