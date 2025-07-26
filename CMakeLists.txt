cmake_minimum_required(VERSION 3.10)
project(onedrivecli)

set(CMAKE_CXX_STANDARD 17)

# Use vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(onedrivecli main.cpp auth.cpp)
target_link_libraries(onedrivecli PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)