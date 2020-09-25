cmake_minimum_required(VERSION 2.8.0)
project(eloapp)

set(TARGET eloapp)

set(WT_DIRECTORY "/usr" CACHE STRING "Path to the Wt deployment directory")
set(CMAKE_PREFIX_PATH "${DIRECTORY}/lib/cmake/")

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

find_package(Wt REQUIRED)
find_package(Boost REQUIRED)

include_directories("${WT_DIRECTORY}/include")
link_directories("${WT_DIRECTORY}/lib")

add_executable(${TARGET}
    main.cpp
    rankingwidget.cpp
    rankingwidget.hpp
    database.cpp
    database.hpp
    database_orm.hpp
)

target_link_libraries(${TARGET} wt wthttp wtdbo wtdbosqlite3)
