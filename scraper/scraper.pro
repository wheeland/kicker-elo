TARGET = scraper
CONFIG += c++11
TEMPLATE = app

QT += core network sql

SOURCES += \
    main.cpp \
    downloader.cpp \
    database.cpp \
    league.cpp \
    tournament.cpp \
    scrapeutil.cpp \
    \
    gumbo-parser/src/attribute.c \
    gumbo-parser/src/char_ref.c \
    gumbo-parser/src/error.c \
    gumbo-parser/src/parser.c \
    gumbo-parser/src/string_buffer.c \
    gumbo-parser/src/string_piece.c \
    gumbo-parser/src/tag.c \
    gumbo-parser/src/tokenizer.c \
    gumbo-parser/src/utf8.c \
    gumbo-parser/src/util.c \
    gumbo-parser/src/vector.c \

HEADERS += \
    downloader.hpp \
    database.hpp \
    league.hpp \
    tournament.hpp \
    scrapeutil.hpp

INCLUDEPATH += \
    gumbo-parser/src/
