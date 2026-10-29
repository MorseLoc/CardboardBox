cmake_minimum_required(VERSION 3.25)
project(FemboyLover VERSION 0.2.0 LANGUAGES CXX)

find_package(SDL3 3.2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_library(ClubhouseWorld STATIC
    src/cards/Tabletop.cpp
    src/cards/TabletopController.cpp
    src/world/Geometry.cpp
    src/world/Room.cpp
    src/world/Player.cpp
)
target_compile_features(ClubhouseWorld PUBLIC cxx_std_20)
target_include_directories(ClubhouseWorld PUBLIC src)
set_target_properties(ClubhouseWorld PROPERTIES CXX_EXTENSIONS OFF)

add_executable(FemboyLover
    src/main.cpp
    src/core/Application.cpp
    src/cards/CardArt.cpp
    src/graphics/BitmapFont.cpp
    src/graphics/Camera.cpp
    src/graphics/GlApi.cpp
    src/graphics/Renderer.cpp
)
target_compile_features(FemboyLover PRIVATE cxx_std_20)
set_target_properties(FemboyLover PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(FemboyLover PRIVATE src)
target_link_libraries(FemboyLover PRIVATE ClubhouseWorld SDL3::SDL3 glm::glm OpenGL::GL)
target_compile_definitions(FemboyLover PRIVATE SDL_MAIN_HANDLED GLM_FORCE_RADIANS NOMINMAX WIN32_LEAN_AND_MEAN)
if(MSVC)
    target_compile_options(ClubhouseWorld PRIVATE /W4 /permissive- /utf-8 /EHsc)
    target_compile_options(FemboyLover PRIVATE /W4 /permissive- /utf-8 /EHsc)
else()
    target_compile_options(ClubhouseWorld PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(FemboyLover PRIVATE -Wall -Wextra -Wpedantic)
endif()
# Keep a console in this development milestone so startup diagnostics are visible.
# Copy SDL beside the executable: do not rely on a developer's PATH at runtime.
if(WIN32)
    add_custom_command(TARGET FemboyLover POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:FemboyLover>
        VERBATIM)
endif()

# These headless tests exercise real movement/seating logic, not rendering.
include(CTest)
if(BUILD_TESTING)
    add_executable(RoomTests tests/RoomTests.cpp src/graphics/BitmapFont.cpp)
    target_link_libraries(RoomTests PRIVATE ClubhouseWorld)
    set_target_properties(RoomTests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME clubhouse-world COMMAND RoomTests)
    add_executable(CardTests tests/CardTests.cpp src/cards/CardArt.cpp src/graphics/BitmapFont.cpp)
    target_link_libraries(CardTests PRIVATE ClubhouseWorld)
    set_target_properties(CardTests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME physical-tabletop COMMAND CardTests)
    add_executable(CameraTests tests/CameraTests.cpp src/graphics/Camera.cpp)
    target_link_libraries(CameraTests PRIVATE ClubhouseWorld glm::glm)
    target_compile_definitions(CameraTests PRIVATE GLM_FORCE_RADIANS)
    set_target_properties(CameraTests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME camera-rays COMMAND CameraTests)
endif()
