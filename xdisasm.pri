INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/dialogdisasmlabels.cpp \
    $$PWD/dialogdisasmprocess.cpp \
    $$PWD/xdisasm.cpp \
    $$PWD/xdisasmmodel.cpp \
    $$PWD/xdisasmwidget.cpp

HEADERS += \
    $$PWD/dialogdisasmlabels.h \
    $$PWD/dialogdisasmprocess.h \
    $$PWD/xdisasm.h \
    $$PWD/xdisasmmodel.h \
    $$PWD/xdisasmwidget.h

FORMS += \
    $$PWD/dialogdisasmlabels.ui \
    $$PWD/dialogdisasmprocess.ui \
    $$PWD/xdisasmwidget.ui

include(../XCapstone/xcapstone.pri)
include(../Formats/xpe.pri)
