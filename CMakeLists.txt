cmake_minimum_required(VERSION 3.21)

project(hdag VERSION 1 DESCRIPTION "Hash DAG database library and tools")
include(CTest)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRAPHVIZ REQUIRED libcgraph)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include_directories(include ${GRAPHVIZ_INCLUDE_DIRS})

add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror
)

add_definitions(-D_GNU_SOURCE)

add_library(
    hdag
    lib/hdag/dot.c
    lib/hdag/bundle.c
    lib/hdag/file.c
    lib/hdag/node_seq.c
    lib/hdag/hash_seq.c
    lib/hdag/nodes.c
    lib/hdag/node.c
    lib/hdag/darr.c
    lib/hdag/hash.c
    lib/hdag/misc.c
    lib/hdag/rc.c
)
target_link_libraries(hdag ${GRAPHVIZ_LIBRARIES})

add_executable(hdagt-rc src/hdagt/hdagt-rc.c)
target_link_libraries(hdagt-rc hdag)
add_test(NAME rc COMMAND hdagt-rc)

add_executable(hdagt-file src/hdagt/hdagt-file.c)
target_link_libraries(hdagt-file hdag)
add_test(NAME file COMMAND hdagt-file)

add_executable(hdagt-bundle src/hdagt/hdagt-bundle.c)
target_link_libraries(hdagt-bundle hdag)
add_test(NAME bundle COMMAND hdagt-bundle)

set_target_properties(hdag PROPERTIES VERSION ${PROJECT_VERSION})

install(
    TARGETS hdag
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
