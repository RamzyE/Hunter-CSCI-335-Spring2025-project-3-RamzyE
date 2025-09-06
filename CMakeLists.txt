cmake_minimum_required(VERSION 3.16)
project(335_project2)

# Set to c++17
set(CMAKE_CXX_STANDARD 17)

# Define source files for the main executable
set(SOURCES main.cpp Leaderboard.cpp PlayerStream.cpp)

# Create the executable
add_executable(main ${SOURCES})

# Disable SSL before fetching CPR
set(CPR_ENABLE_SSL OFF CACHE BOOL "Enables or disables the SSL backend." FORCE)

# Fetch CPR Library for cURL
include(FetchContent)
FetchContent_Declare(cpr
                    GIT_REPOSITORY https://github.com/libcpr/cpr.git
                    GIT_TAG 1.11.0) # The commit hash for 1.11.x
FetchContent_MakeAvailable(cpr)

# Fetch JSON parsing library to read responses
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(main
    PRIVATE 
    cpr::cpr 
    nlohmann_json::nlohmann_json
)