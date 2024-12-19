cmake_minimum_required(VERSION 3.12)

project(chess_agent)

if(WIN32)
    # this is to fix some DLL dependency issues, Just copy the dependency into the compiled
    # binaries as a static library
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

    # I did this for my specific machine, but typically, you will want to search your target
    # version of python regardless of the OS
    set(PYTHON_ROOT_DIR "C:/Users/user/AppData/Local/Programs/Python/Python311")

    set(PYTHON_EXECUTABLE "C:/Users/user/AppData/Local/Programs/Python/Python311/python.exe")
endif()

include_directories(${PYTHON_INCLUDE_DIRS} include)

add_subdirectory(extern/pybind11)

add_compile_options(-O3 -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fvisibility-inlines-hidden -nodefaultlibs -nostartfiles -static)

add_link_options("-Wl,--gc-sections" "-Bsymbolic" "-Wl,--exclude-libs, ALL")

pybind11_add_module(chess_agent binder.cpp)

add_custom_command(TARGET chess_agent POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-debug $<TARGET_FILE:chess_agent>)