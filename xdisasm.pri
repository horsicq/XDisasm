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

!contains(XCONFIG, xcapstone) {
    XCONFIG += xcapstone
    include(../XCapstone/xcapstone.pri)
}

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include(../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, xlineedithex) {
    XCONFIG += xlineedithex
    include(../Controls/xlineedithex.pri)
}

!contains(XCONFIG, xpe) {
    XCONFIG += xpe
    include(../Formats/xpe.pri)
}

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include(../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, dialogdump) {
    XCONFIG += dialogdump
    include(../FormatDialogs/dialogdump.pri)
}
