cmake_minimum_required(VERSION 3.16)
project(awesome)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
#find_package(Vulkan REQUIRED)

include(FindOpenGL)
pkg_check_modules(sdl2 sdl2)
pkg_check_modules(libevent REQUIRED libevent)
pkg_check_modules(libevent_pthreads REQUIRED libevent_pthreads)
pkg_check_modules(libgeek REQUIRED libgeek)
find_path(XF86_INCLUDE_PATH xf86drm.h)
find_library(LIBRT rt)

add_definitions(${libgeek_CFLAGS} ${libevent_CFLAGS} ${sdl2_CFLAGS})
include_directories(include)

add_definitions(-DAWESOME_SRC=${PROJECT_SOURCE_DIR})

list(APPEND ads_srcs
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
    src/ads/drivers/opengl.cpp
    src/ads/drivers/opengl.h
    src/ads/interface.cpp
    src/ads/interfaces/awesome/awesome.cpp
    src/ads/interfaces/awesome/client.cpp
    src/ads/interfaces/awesome/awesome.h
)

set(DRIVER_LIBRARIES "")

if (sdl2_FOUND)
    list(APPEND ads_srcs
        src/ads/drivers/sdl/sdl.cpp
        src/ads/drivers/sdl/sdl.h
    )
    list(APPEND DRIVER_LIBRARIES ${sdl2_LDFLAGS})
    add_definitions(-DAWESOME_ENGINE_SDL=1)
endif()
if (XF86_INCLUDE_PATH)
    pkg_check_modules(gbm REQUIRED gbm)
    pkg_check_modules(egl REQUIRED egl)
    list(APPEND ads_srcs
            src/ads/drivers/linuxdrm/linuxdrm.cpp
            src/ads/drivers/linuxdrm/linuxdrm.h
    )
    add_definitions(-I${XF86_INCLUDE_PATH}/drm -DAWESOME_ENGINE_DRM=1)
    list(APPEND DRIVER_LIBRARIES -ldrm ${gbm_LDFLAGS} ${egl_LDFLAGS} -lrt)
endif()

add_executable(awesome-display-server ${ads_srcs})
target_include_directories(awesome-display-server PRIVATE ${PROJECT_SOURCE_DIR}/src/ads)
target_link_libraries(awesome-display-server ${libgeek_LDFLAGS} ${DRIVER_LIBRARIES} ${OPENGL_LIBRARIES} ${libevent_LDFLAGS} ${libevent_pthreads_LDFLAGS})

install(TARGETS awesome-display-server DESTINATION bin)

#
# libawesome
#
add_library(awesome SHARED
        src/libawesome/connection.cpp
        src/libawesome/connthread.cpp
        )
list(APPEND awesome_LDFLAGS ${libgeek_LDFLAGS})
if (LIBRT)
    list(APPEND awesome_LDFLAGS -lrt)
endif()
target_link_libraries(awesome ${awesome_LDFLAGS})
install(DIRECTORY include/awesome DESTINATION include FILES_MATCHING PATTERN "*.h" )
install(TARGETS awesome DESTINATION lib)

#
# awesome-desktop
#
add_executable(
        awesome-desktop
        src/awesome-desktop/main.cpp
)
target_link_libraries(awesome-desktop awesome)

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
