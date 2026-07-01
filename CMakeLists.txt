cmake_minimum_required(VERSION 3.20)

project(LinuxFileOrganizer LANGUAGES CXX)

add_subdirectory(external/FileCategoryLibrary)

add_executable(FileOrganizer
    src/main.cpp
)

target_compile_features(FileOrganizer PRIVATE cxx_std_17)

target_link_libraries(FileOrganizer
    PRIVATE
        FileCategoryLibrary
)