cmake_minimum_required(VERSION 3.28)
project(CrowRestApi)

# Setze den C++ Standard auf C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Füge das ausführbare Ziel hinzu
add_executable(CrowRestApi main.cpp)

# Inkludiere das Crow- und Asio-Verzeichnis
include_directories("/home/pascal/Übungen/Übung/REST_API_Übung/my_crow_api/Crow/include")
include_directories("/home/pascal/Übungen/Übung/REST_API_Übung/my_crow_api/asio/include")

# Füge den Pfad zur nlohmann/json-Bibliothek hinzu
include_directories("/home/pascal/Übungen/Übung/REST_API_Übung/my_crow_api/Crow/include/nlohmann")

# Optional: Falls du CMake zur Verwaltung von Abhängigkeiten verwenden möchtest
# find_package(Threads REQUIRED)  # Wenn du die Thread-Bibliothek benötigst (z.B. bei Asio)
# target_link_libraries(CrowRestApi PRIVATE Threads::Threads)
