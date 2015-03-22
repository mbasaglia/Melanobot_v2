# Tries to find the Iconv library

set(Iconv_PREFIX "${CMAKE_SYSTEM_PREFIX_PATH}" CACHE PATH "Iconv installation prefix")
set(Iconv_HEADER_BASENAME iconv.h)

find_path(Iconv_INCLUDE_DIR ${Iconv_HEADER_BASENAME} PATHS ${Iconv_PREFIX})
find_library(Iconv_LIBRARY NAMES iconv PATHS ${Iconv_PREFIX})

set(Iconv_ERROR_MESSAGE "")
if(Iconv_INCLUDE_DIR)
    set(Iconv_FOUND TRUE)
else()
    set(Iconv_ERROR_MESSAGE "Iconv not found!")
endif()

if(Iconv_FOUND)
    message(STATUS "Found Iconv")
else()
    if(Iconv_FIND_REQUIRED)
        message(FATAL_ERROR ${Iconv_ERROR_MESSAGE})
    elseif(NOT Iconv_FIND_QUIETLY)
        message(STATUS ${Iconv_ERROR_MESSAGE})
    endif()
endif()
