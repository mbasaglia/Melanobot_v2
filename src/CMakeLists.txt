file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)
set(EXECUTABLE melanobot)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")


add_executable(${EXECUTABLE} ${SOURCES})
install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)

find_package (Boost COMPONENTS system chrono REQUIRED)
target_link_libraries(${EXECUTABLE} ${Boost_LIBRARIES})
include_directories (${Boost_INCLUDE_DIRS})
