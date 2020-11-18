cmake_minimum_required(VERSION 3.16)
project(awesome)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
#find_package(Vulkan REQUIRED)

#pkg_check_modules(glfw3 REQUIRED glfw3)
include(FindOpenGL)
include(FindGLUT)
pkg_check_modules(sdl2 sdl2)
pkg_check_modules(libevent REQUIRED libevent)
pkg_check_modules(libgeek REQUIRED libgeek)

add_definitions(${libgeek_CFLAGS} ${libevent_CFLAGS} ${sdl2_CFLAGS})
include_directories(include)

add_definitions(-DAWESOME_SRC=${PROJECT_SOURCE_DIR})

add_executable(awesome-display-server
    include/awesome/displaydriver.h
    include/awesome/displayserver.h
    include/awesome/interface.h
    include/awesome/window.h
    include/awesome/compositor.h
    include/awesome/protocol.h
    src/ads/main.cpp
    src/ads/client.cpp
    src/ads/compositor.cpp
    src/ads/displayserver.cpp
    src/ads/displaydriver.cpp
    src/ads/display.cpp
    src/ads/window.cpp
    src/ads/interface.cpp
    src/ads/drivers/sdl/sdl.cpp
    src/ads/drivers/sdl/sdl.h
    src/ads/interfaces/awesome/awesome.cpp
    src/ads/interfaces/awesome/client.cpp
    src/ads/interfaces/awesome/awesome.h
)
target_link_libraries(awesome-display-server ${libgeek_LDFLAGS} ${sdl2_LDFLAGS} ${OPENGL_LIBRARIES} ${libevent_LDFLAGS})

install(TARGETS awesome-display-server DESTINATION bin)

#
# libawesome
#
add_library(awesome SHARED src/libawesome/connection.cpp)
target_link_libraries(awesome ${libgeek_LDFLAGS})
install(DIRECTORY include/awesome DESTINATION include FILES_MATCHING PATTERN "*.h" )
install(TARGETS awesome DESTINATION lib)

#
# ads-test
#
add_executable(
        ads-test
        src/ads-test/main.cpp
)
target_link_libraries(ads-test awesome)

#add_executable(awesome-vulkan src/vulkan/main.cpp src/vulkan/vulkanengine.cpp src/vulkan/vulkanengine.h)
#target_link_libraries(awesome-vulkan ${glfw3_LDFLAGS} ${libgeek_LDFLAGS} ${Vulkan_LIBRARY})


#
# libawesome.pc
#
set(PKGCONFIG_CFLAGS_LIST -I${CMAKE_INSTALL_PREFIX}/include ${libgeek_CFLAGS})
string(REPLACE ";" " " PKGCONFIG_CFLAGS "${PKGCONFIG_CFLAGS_LIST}")
set(PKGCONFIG_LDFLAGS_LIST -L${CMAKE_INSTALL_PREFIX}/lib -lawesome ${libgeek_LDFLAGS})
string(REPLACE ";" " " PKGCONFIG_LDFLAGS "${PKGCONFIG_LDFLAGS_LIST}")

configure_file(libawesome.pc.in libawesome.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libawesome.pc DESTINATION "lib${LIB_SUFFIX}/pkgconfig")
