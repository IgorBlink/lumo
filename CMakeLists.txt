cmake_minimum_required(VERSION 3.14)
project(LumoCompiler VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ── Dependencies ─────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# ── Sources ──────────────────────────────────────────────────────────────────
include_directories(include)
include_directories(vendor)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(lumo ${SOURCES})
target_link_libraries(lumo PRIVATE nlohmann_json::nlohmann_json)
