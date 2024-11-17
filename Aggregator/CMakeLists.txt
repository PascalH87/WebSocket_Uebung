cmake_minimum_required(VERSION 3.10)
project(CrowWebSocketServer)

# Pfad zur vcpkg Installation
set(CMAKE_TOOLCHAIN_FILE "/home/pascal/Oszilo_Socket/Aufgabe5/Server3/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# C++17 aktivieren
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Setze den CMAKE_PREFIX_PATH für vcpkg
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-linux/share" CACHE STRING "Path to vcpkg installed packages")

# Crow und nlohmann-json finden
find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Threads-Bibliothek finden
find_package(Threads REQUIRED)

# Manuelles Hinzufügen von UUID (falls find_package UUID fehlschlägt)
find_path(UUID_INCLUDE_DIR uuid/uuid.h)
find_library(UUID_LIBRARY NAMES uuid)

if (UUID_INCLUDE_DIR AND UUID_LIBRARY)
    message(STATUS "UUID found: ${UUID_INCLUDE_DIR} ${UUID_LIBRARY}")
else()
    message(FATAL_ERROR "UUID not found")
endif()

# Ziel erstellen und Quellen hinzufügen
add_executable(server server.cpp)

# Crow und nlohmann-json zum Projekt verlinken
target_link_libraries(server PRIVATE Crow::Crow nlohmann_json::nlohmann_json)

# Verlinke mit der UUID-Bibliothek und pthread (statt Threads)
target_link_libraries(server PRIVATE ${UUID_LIBRARY} pthread)

# Füge den UUID-Include-Pfad hinzu
target_include_directories(server PRIVATE ${UUID_INCLUDE_DIR})

# Zusätzliche Include-Pfade, falls notwendig
target_include_directories(server PRIVATE "${CMAKE_SOURCE_DIR}/include")

