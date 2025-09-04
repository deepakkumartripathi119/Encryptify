cmake_minimum_required(VERSION 3.10)
project(Encryptify)

set(CMAKE_CXX_STANDARD 17)

# Use the vcpkg-specific find_package name
find_package(unofficial-sodium CONFIG REQUIRED)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(vault ${SOURCES})

# Use the vcpkg-specific target name
target_link_libraries(vault PRIVATE unofficial-sodium::sodium)