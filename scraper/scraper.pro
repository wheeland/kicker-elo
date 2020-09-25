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
    rating.cpp \
    \
    ../3rdparty/gumbo-parser/src/attribute.c \
    ../3rdparty/gumbo-parser/src/char_ref.c \
    ../3rdparty/gumbo-parser/src/error.c \
    ../3rdparty/gumbo-parser/src/parser.c \
    ../3rdparty/gumbo-parser/src/string_buffer.c \
    ../3rdparty/gumbo-parser/src/string_piece.c \
    ../3rdparty/gumbo-parser/src/tag.c \
    ../3rdparty/gumbo-parser/src/tokenizer.c \
    ../3rdparty/gumbo-parser/src/utf8.c \
    ../3rdparty/gumbo-parser/src/util.c \
    ../3rdparty/gumbo-parser/src/vector.c \

HEADERS += \
    downloader.hpp \
    database.hpp \
    league.hpp \
    tournament.hpp \
    scrapeutil.hpp \
    rating.hpp \

INCLUDEPATH += \
    ../3rdparty/gumbo-parser/src/
