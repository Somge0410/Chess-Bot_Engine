# Sets the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Sets the project name
project(Chess-Bot_Engine)

# --- ADD THESE TWO LINES HERE ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# --------------------------------

# Tells CMake to create an executable from all your .cpp files
add_executable(Chess-Bot_Engine
    src/board.cpp
    src/engine.cpp
    src/evaluation.cpp
    #src/generator.cpp
    src/main.cpp
    src/MoveGenerator.cpp
    src/notation_utils.cpp
    src/zobrist.cpp
      "include/rook_tables.h" "include/prepare_data.h" "include/uci_helpers.h" "src/uci.cpp" "include/uci.h")

# Tells the compiler where to find your .h header files
target_include_directories(Chess-Bot_Engine PUBLIC include)


# --- ADD THIS LINE FOR THE PROFILER ---
target_link_options(Chess-Bot_Engine PRIVATE /PROFILE)