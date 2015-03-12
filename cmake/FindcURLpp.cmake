# Tries to find the cURLpp library

set(cURLpp_PREFIX "${CMAKE_SYSTEM_PREFIX_PATH}" CACHE PATH "cURLpp installation prefix")
set(cURLpp_HEADER_BASENAME curlpp/cURLpp.hpp)

find_path(cURLpp_INCLUDE_DIR ${cURLpp_HEADER_BASENAME} PATHS ${cURLpp_PREFIX})
find_library(cURLpp_LIBRARY NAMES curlpp PATHS ${cURLpp_PREFIX})
find_library(cURL_LIBRARY NAMES curl PATHS ${cURLpp_PREFIX})

set(cURLpp_ERROR_MESSAGE "")
if(cURLpp_LIBRARY AND cURLpp_INCLUDE_DIR)
    set(cURLpp_FOUND TRUE)

    file(READ "${cURLpp_INCLUDE_DIR}/${cURLpp_HEADER_BASENAME}" cURLpp_HEADER_CONTENTS)
    string(REGEX MATCH "#define LIBCURLPP_VERSION \"([0-9.]+)\".*" cURLpp_HEADER_CONTENTS "${cURLpp_HEADER_CONTENTS}")
    string(REGEX REPLACE "#define LIBCURLPP_VERSION \"([0-9.]+)\".*" "\\1" cURLpp_VERSION_STRING "${cURLpp_HEADER_CONTENTS}")
    set(cURLpp_LIBRARIES ${cURLpp_LIBRARY} ${cURL_LIBRARY})
    if (cURLpp_FIND_VERSION AND cURLpp_FIND_VERSION STRGREATER cURLpp_VERSION_STRING)
        set(cURLpp_FOUND FALSE)
        set(cURLpp_ERROR_MESSAGE "cURLpp not found (required was ${cURLpp_FIND_VERSION} but found ${cURLpp_VERSION_STRING})")
    endif()
else()
	set(cURLpp_ERROR_MESSAGE "cURLpp not found!")
endif()

if(cURLpp_FOUND)
    message(STATUS "cURLpp Version: ${cURLpp_VERSION_STRING}")
else()
    if(cURLpp_FIND_REQUIRED)
        message(FATAL_ERROR ${cURLpp_ERROR_MESSAGE})
    elseif(NOT cURLpp_FIND_QUIETLY)
        message(STATUS ${cURLpp_ERROR_MESSAGE})
    endif()
endif()
