cmake_minimum_required(VERSION 3.22)
project(smart_agriculture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Crow CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(include)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(FATAL_ERROR "请在Windows下使用MSVC编译器")
    else()
        if(NOT DEFINED WIN32_WINNT)
            # 获取 Windows 系统的版本
            string(REGEX MATCH "([0-9]+)" WIN_VERSION ${CMAKE_SYSTEM_VERSION})
            # 判断 Windows 版本并设置 _WIN32_WINNT
            if(WIN_VERSION VERSION_LESS "10")
                # Windows 7
                add_definitions(-D_WIN32_WINNT=0x0601)
            elseif(WIN_VERSION VERSION_LESS "11")
                # Windows 10
                add_definitions(-D_WIN32_WINNT=0x0A00)
            else()
                # Windows 11
                add_definitions(-D_WIN32_WINNT=0x0A01)
            endif()
        endif()
        add_executable(smart_agriculture src/main.cpp)

        target_link_libraries(smart_agriculture PRIVATE Crow::Crow asio::asio)
        target_link_libraries(smart_agriculture PRIVATE unofficial::sqlite3::sqlite3)
        target_link_libraries(smart_agriculture PRIVATE unofficial::mysql-connector-cpp::connector)
        target_link_libraries(smart_agriculture PRIVATE OpenSSL::Crypto)
        set_target_properties(smart_agriculture PROPERTIES OUTPUT_NAME "smart_agriculture_win_x86-64")
    endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "请在Linux下使用g++编译器")
    else()
        add_executable(smart_agriculture src/main.cpp
        )

        # vcpkg 的库
        target_link_libraries(smart_agriculture PRIVATE Crow::Crow asio::asio)
        target_link_libraries(smart_agriculture PRIVATE unofficial::sqlite3::sqlite3)
        target_link_libraries(smart_agriculture PRIVATE unofficial::mysql-connector-cpp::connector)

        # protobuf 明确链接
        find_package(Protobuf REQUIRED)
        target_link_libraries(smart_agriculture PRIVATE protobuf::libprotobuf)

        # OpenSSL 明确链接
        target_link_libraries(smart_agriculture PRIVATE OpenSSL::Crypto)

        set_target_properties(smart_agriculture PROPERTIES OUTPUT_NAME "smart_agriculture_linux_x86-64")
    endif()
endif()
