TEMPLATE = subdirs

SUBDIRS += \
    Client \
    Protocol \
    QConsoleListener

Client.depends += QConsoleListener
Client.depends += Protocol
Protocol.depends += QConsoleListener
