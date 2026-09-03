cmake_minimum_required(VERSION 3.30)
project(brother-laser-pappl)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PAPPL REQUIRED pappl)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
find_program(CLANG_FORMAT_EXE NAMES clang-format)

set(SOURCES
    src/main.cpp
)

set(MODULE_SOURCES
    src/common.cpp
    src/pappl.cpp
    src/printer.cpp
)

add_executable(brother-laser-pappl ${SOURCES})

target_sources(brother-laser-pappl
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            ${MODULE_SOURCES}
)

if (CLANG_FORMAT_EXE)
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${SOURCES} ${MODULE_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking clang-format on source files"
    )

    add_dependencies(brother-laser-pappl clang-format)
else()
    message(WARNING "clang-format not found, skipping format check")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (NOT DEFINED CMAKE_CXX_CLANG_TIDY)
        if (CLANG_TIDY_EXE)
            set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
        else()
            message(WARNING "clang-tidy not found, skipping static analysis")
        endif()
    endif()

    message(STATUS "clang-tidy set to: ${CMAKE_CXX_CLANG_TIDY}")

    if (CMAKE_CXX_CLANG_TIDY)
        set_target_properties(brother-laser-pappl PROPERTIES CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}")
    endif()

    set_source_files_properties(${SOURCES} ${MODULE_SOURCES} PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/.clang-tidy")
else()
    message(WARNING "clang-tidy is only supported with Clang compiler, skipping static analysis")
endif()

target_compile_options(brother-laser-pappl PRIVATE -O2 -Wall -Wextra -Werror -Wpedantic)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(brother-laser-pappl PRIVATE -Wunsafe-buffer-usage -Wthread-safety)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 23)
        target_compile_options(brother-laser-pappl PRIVATE -Wlifetime-safety-all)
    endif()
endif()

target_link_libraries(brother-laser-pappl PRIVATE ${PAPPL_LIBRARIES})
target_include_directories(brother-laser-pappl PRIVATE ${PAPPL_INCLUDE_DIRS})
install(TARGETS brother-laser-pappl DESTINATION bin)
