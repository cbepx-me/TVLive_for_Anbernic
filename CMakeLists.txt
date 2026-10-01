cmake_minimum_required(VERSION 3.16)
project(tvlive CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2       REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF   REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_GFX   REQUIRED SDL2_gfx)
pkg_check_modules(CURL       REQUIRED libcurl)

add_executable(tvlive
    src/main.cpp
    src/config.cpp
    src/translator.cpp
    src/input.cpp
    src/renderer.cpp
    src/scanner.cpp
    src/player.cpp
    src/logo_loader.cpp
    src/app.cpp
)

target_include_directories(tvlive PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_GFX_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(tvlive
    -l:libSDL2-2.0.so.0
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

install(TARGETS tvlive RUNTIME DESTINATION .)

install(DIRECTORY lang DESTINATION .)

if(EXISTS "${CMAKE_SOURCE_DIR}/font/font.ttf")
    install(FILES font/font.ttf DESTINATION font)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/TV")
    install(DIRECTORY TV DESTINATION .)
endif()