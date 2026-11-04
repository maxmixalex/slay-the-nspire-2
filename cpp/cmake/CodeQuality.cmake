# Native checks use the same compile database as the selected host build.
find_program(SPIRE_CLANG_FORMAT NAMES clang-format clang-format-18)
find_program(SPIRE_CLANG_TIDY NAMES clang-tidy clang-tidy-18)
file(GLOB_RECURSE SPIRE_FORMAT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp")
list(FILTER SPIRE_FORMAT_SOURCES EXCLUDE REGEX "/detail/data/|/fixtures/data/")
if(SPIRE_CLANG_FORMAT)
    add_custom_target(format
        COMMAND "${SPIRE_CLANG_FORMAT}" -i ${SPIRE_FORMAT_SOURCES}
        COMMENT "Formatting native sources" VERBATIM)
    add_custom_target(format-check
        COMMAND "${SPIRE_CLANG_FORMAT}" --dry-run --Werror ${SPIRE_FORMAT_SOURCES}
        COMMENT "Checking native source formatting" VERBATIM)
else()
    message(STATUS "clang-format not found; format and format-check targets unavailable")
endif()
if(SPIRE_CLANG_TIDY)
    set(SPIRE_TIDY_SOURCES ${SPIRE_CORE_SOURCES})
    list(FILTER SPIRE_TIDY_SOURCES INCLUDE REGEX "^src/.*\\.cpp$")
    add_custom_target(tidy
        COMMAND "${SPIRE_CLANG_TIDY}" -p "${CMAKE_BINARY_DIR}" ${SPIRE_TIDY_SOURCES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Analyzing native sources with the configured compile database" VERBATIM)
else()
    message(STATUS "clang-tidy not found; tidy target unavailable")
endif()
