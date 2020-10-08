cmake_minimum_required(VERSION 2.8.0)

project(eloapp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(WT_DIRECTORY "/usr" CACHE STRING "Path to the Wt deployment directory")
SET(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)
find_package(Qt5 COMPONENTS Core Sql REQUIRED)

include_directories("${WT_DIRECTORY}/include")
link_directories("${WT_DIRECTORY}/lib")

set(TARGET eloapp)

add_executable(${TARGET}
    main.cpp
    rankingwidget.cpp
    rankingwidget.hpp
    playerwidget.cpp
    playerwidget.hpp
    database.cpp
    database.hpp
    util.hpp
)

target_link_libraries(${TARGET} wt wthttp Qt5::Core Qt5::Sql)
