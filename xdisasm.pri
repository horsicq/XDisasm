INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += \
    $$PWD/dialogdisasmlabels.cpp \
    $$PWD/dialogdisasmprocess.cpp \
    $$PWD/dialogsignature.cpp \
    $$PWD/xdisasm.cpp \
    $$PWD/xdisasmmodel.cpp \
    $$PWD/xdisasmwidget.cpp

HEADERS += \
    $$PWD/dialogdisasmlabels.h \
    $$PWD/dialogdisasmprocess.h \
    $$PWD/dialogsignature.h \
    $$PWD/xdisasm.h \
    $$PWD/xdisasmmodel.h \
    $$PWD/xdisasmwidget.h

FORMS += \
    $$PWD/dialogdisasmlabels.ui \
    $$PWD/dialogdisasmprocess.ui \
    $$PWD/dialogsignature.ui \
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
    include($$PWD/../QhexView/qhexview.pri)
}
