TEMPLATE = subdirs

SUBDIRS += \
    Client \
    Protocol \
    QConsoleListener \
    Server

Client.depends += Protocol
Server.depends += Protocol
Protocol.depends += QConsoleListener
