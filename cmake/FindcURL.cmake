# Tries to find the cURL library

set(cURL_PREFIX "${CMAKE_SYSTEM_PREFIX_PATH}" CACHE PATH "cURL installation prefix")
set(cURL_HEADER_BASENAME curl/curl.h)

find_path(cURL_INCLUDE_DIR ${cURL_HEADER_BASENAME} PATHS ${cURL_PREFIX})
find_library(cURL_LIBRARY NAMES curl PATHS ${cURL_PREFIX})

set(cURL_ERROR_MESSAGE "")
if(cURL_LIBRARY AND cURL_INCLUDE_DIR)
    set(cURL_FOUND TRUE)

    execute_process(
        COMMAND curl-config --version | grep -E "[0-9.]+"
        OUTPUT_VARIABLE cURL_VERSION_STRING
        ERROR_QUIET
    )
    set(cURL_LIBRARIES ${cURL_LIBRARY})
    if (cURL_FIND_VERSION AND cURL_FIND_VERSION STRGREATER cURL_VERSION_STRING)
        set(cURL_FOUND FALSE)
        set(cURL_ERROR_MESSAGE "cURL not found (required was ${cURL_FIND_VERSION} but found ${cURL_VERSION_STRING})")
    endif()
else()
    set(cURL_ERROR_MESSAGE "cURL not found!")
endif()

if(cURL_FOUND)
    message(STATUS "cURL Version: ${cURL_VERSION_STRING}")
else()
    if(cURL_FIND_REQUIRED)
        message(FATAL_ERROR ${cURL_ERROR_MESSAGE})
    elseif(NOT cURL_FIND_QUIETLY)
        message(STATUS ${cURL_ERROR_MESSAGE})
    endif()
endif()
