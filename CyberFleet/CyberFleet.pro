TEMPLATE = subdirs

SUBDIRS += \
    ClientGUI \
    FactorySlot \
    Protocol \
    QConsoleListener \
    Server

ClientGUI.depends += Protocol
ClientGUI.depends += FactorySlot
Server.depends += Protocol
Protocol.depends += QConsoleListener
