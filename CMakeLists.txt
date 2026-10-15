cmake_minimum_required(VERSION 3.21)

project(file_manager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(file_manager WIN32
    src/main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/dired/DirEntry.h
    src/dired/DirModel.h
    src/dired/DirModel.cpp
    src/dired/DiredBuffer.h
    src/dired/DiredBuffer.cpp
    src/dired/DiredHighlighter.h
    src/dired/DiredHighlighter.cpp
    src/dired/KeyMap.h
    src/dired/KeyMap.cpp
    src/dired/Commands.h
    src/dired/Commands.cpp
    src/editor/EditorBuffer.h
    src/editor/EditorBuffer.cpp
    src/editor/EditorCommands.h
    src/editor/EditorCommands.cpp
    src/ui/Minibuffer.h
    src/ui/Minibuffer.cpp
    src/ui/BlockCursor.h
    src/ui/BlockCursor.cpp
    src/ui/Theme.h
    src/ui/Theme.cpp
    src/util/FsOps.h
    src/util/FsOps.cpp
    src/util/Format.h
    src/util/Format.cpp
)

target_include_directories(file_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(file_manager PRIVATE Qt6::Widgets)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(file_manager PRIVATE -Wall -Wextra)
elseif(MSVC)
    target_compile_options(file_manager PRIVATE /W4)
endif()
