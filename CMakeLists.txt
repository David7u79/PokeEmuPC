cmake_minimum_required(VERSION 3.20)
project(PocketPartner VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Locate Qt 6
if(NOT Qt6_DIR AND EXISTS "C:/Qt/6.7.2/msvc2019_64")
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.7.2/msvc2019_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Test)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Subdirectories
add_subdirectory(storage)
add_subdirectory(core)
add_subdirectory(emulator)
add_subdirectory(save)
add_subdirectory(companion)
add_subdirectory(desktop_companion)
add_subdirectory(app)

enable_testing()
add_subdirectory(tests)
