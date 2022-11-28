QT += network widgets

HEADERS       = server.h \
    qsslserver.h
SOURCES       = server.cpp \
                main.cpp \
                qsslserver.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/fortuneserver
INSTALLS += target
