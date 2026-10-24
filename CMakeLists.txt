cmake_minimum_required(VERSION 3.16)
project(cvrp_bioinspirados CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(highs REQUIRED)
find_package(Threads REQUIRED)

add_executable(cvrp
  src/main.cpp
  src/instancia.cpp
  src/algoritmo.cpp
  src/modelo.cpp
  src/geneticos.cpp
  src/hormigas.cpp
  src/experimentos.cpp
  src/tablas.cpp
)
target_include_directories(cvrp PRIVATE include)
target_compile_options(cvrp PRIVATE -Wall -Wextra)
target_link_libraries(cvrp PRIVATE highs::highs Threads::Threads)
