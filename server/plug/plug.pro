#-------------------------------------------------
#
# Project created by QtCreator 2010-12-13T13:30:06
#
#-------------------------------------------------

QT       += core widgets gui network

TARGET = plug
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -pedantic -Wall -Wextra
QMAKE_CXXFLAGS_RELEASE += -O2 -DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT
QMAKE_CXXFLAGS_DEBUG += -ggdb -g3 -O0

SOURCES += main.cpp\
        mainwindow.cpp \
    effect.cpp \
    amplifier.cpp \
    mustang.cpp \
    amp_advanced.cpp \
    saveonamp.cpp \
    loadfromamp.cpp \
    save_effects.cpp \
    settings.cpp \
    loadfromfile.cpp \
    savetofile.cpp \
    library.cpp \
    defaulteffects.cpp \
    quickpresets.cpp

HEADERS  += mainwindow.h \
    effect.h \
    amplifier.h \
    mustang.h \
    effects_enum.h \
    amp_advanced.h \
    data_structs.h \
    saveonamp.h \
    loadfromamp.h \
    save_effects.h \
    settings.h \
    loadfromfile.h \
    savetofile.h \
    library.h \
    defaulteffects.h \
    quickpresets.h

FORMS    += mainwindow.ui \
    effect.ui \
    amplifier.ui \
    amp_advanced.ui \
    saveonamp.ui \
    loadfromamp.ui \
    save_effects.ui \
    settings.ui \
    savetofile.ui \
    library.ui \
    defaulteffects.ui \
    quickpresets.ui

LIBS += -lusb-1.0

 target.path += /usr/local/bin
 INSTALLS += target
