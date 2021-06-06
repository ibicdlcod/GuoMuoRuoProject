TEMPLATE = subdirs

SUBDIRS += \
    QConsoleListener \
    Client \
    Server \
    UIv2

Client.depends = QConsoleListener
Server.depends = QConsoleListener
UIv2.depends = QConsoleListener
