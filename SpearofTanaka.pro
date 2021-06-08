TEMPLATE = subdirs

SUBDIRS += \
    Magic \
    QConsoleListener \
    Client \
    Server \
    UIv2 \
    Magic

Client.depends = QConsoleListener
Server.depends = QConsoleListener
UIv2.depends = QConsoleListener
UIv2.depends = Magic
