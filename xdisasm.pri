INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/dialogdisasm.cpp \
    $$PWD/dialogdisasmlabels.cpp \
    $$PWD/dialogdisasmprocess.cpp \
    $$PWD/dialogasmsignature.cpp \
    $$PWD/xdisasm.cpp \
    $$PWD/xdisasmmodel.cpp \
    $$PWD/xdisasmwidget.cpp

HEADERS += \
    $$PWD/dialogdisasm.h \
    $$PWD/dialogdisasmlabels.h \
    $$PWD/dialogdisasmprocess.h \
    $$PWD/dialogasmsignature.h \
    $$PWD/xdisasm.h \
    $$PWD/xdisasmmodel.h \
    $$PWD/xdisasmwidget.h

FORMS += \
    $$PWD/dialogdisasm.ui \
    $$PWD/dialogdisasmlabels.ui \
    $$PWD/dialogdisasmprocess.ui \
    $$PWD/dialogasmsignature.ui \
    $$PWD/xdisasmwidget.ui

!contains(XCONFIG, xcapstone) {
    XCONFIG += xcapstone
    include($$PWD/../XCapstone/xcapstone.pri)
}

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include($$PWD/../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, xlineedithex) {
    XCONFIG += xlineedithex
    include($$PWD/../Controls/xlineedithex.pri)
}

!contains(XCONFIG, xformats) {
    XCONFIG += xformats
    include($$PWD/../Formats/xformats.pri)
}

!contains(XCONFIG, dialoggotoaddress) {
    XCONFIG += dialoggotoaddress
    include($$PWD/../FormatDialogs/dialoggotoaddress.pri)
}

!contains(XCONFIG, dialogdump) {
    XCONFIG += dialogdump
    include($$PWD/../FormatDialogs/dialogdump.pri)
}

!contains(XCONFIG, qhexview) {
    XCONFIG += qhexview
    include($$PWD/../QHexView/qhexview.pri)
}

!contains(XCONFIG, xoptions) {
    XCONFIG += xoptions
    include($$PWD/../XOptions/xoptions.pri)
}
