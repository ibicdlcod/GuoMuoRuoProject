TEMPLATE = subdirs

SUBDIRS += \
    Client \
    ClientGUI \
    FactorySlot \
    Protocol \
    QConsoleListener \
    Server

Client.depends += Protocol
ClientGUI.depends += Protocol
Server.depends += Protocol
Protocol.depends += QConsoleListener
