cmake_minimum_required(VERSION 3.18)
project(multimigration)

set(CMAKE_CXX_STANDARD 20)
find_package(PostgreSQL 13)

add_executable(multimigration main.cpp)

if (${PostgreSQL_FOUND})
   target_link_libraries(multimigration PRIVATE PostgreSQL::PostgreSQL)
endif ()

