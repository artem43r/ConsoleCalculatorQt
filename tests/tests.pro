QT -= gui
QT += core concurrent testlib

CONFIG += c++17 console testcase
CONFIG -= app_bundle

TARGET = CalculatorTests
TEMPLATE = app

# Тестируем тот же класс, что и в приложении
INCLUDEPATH += ..

SOURCES += \
    tst_calculator.cpp \
    ../calculator.cpp

HEADERS += \
    ../calculator.h
