QT -= gui
QT += core
QT += concurrent

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = ConsoleCalculator
TEMPLATE = app

SOURCES += \
    main.cpp \
    calculator.cpp

HEADERS += \
    calculator.h
