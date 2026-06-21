cmake_minimum_required(VERSION 3.16)
project(CowCam LANGUAGES C CXX)

# Build Release by default
#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
#endif()

# Define flags for 'Release with Debug Info'
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-ggdb -m64 -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-ggdb -m64 -D_FILE_OFFSET_BITS=64")

# Optional: Make this the default if nothing else is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type" FORCE)
endif()

# --- Configuration Options ---
option(USE_CEF "Build CEF HTML Window" ON)
option(USE_OSG "Build OSG Renderer" ON)

set(CEF_PATH "/home/laser/Downloads/OffScreenCEF/thirdparty/cef_binary")
set(CMAKE_CXX_STANDARD 17)

# --- Global Compiler Flags ---
# Replicating MARCH and DBUG
add_compile_options(-m64 -ggdb -D_FILE_OFFSET_BITS=64 -DDEBUG=1)
add_compile_options(-fno-diagnostics-color -Wno-deprecated-declarations -Wno-unused-result -Wno-write-strings)
add_definitions(-DFLTK_HAVE_CAIRO -D_GNU_SOURCE -D_REENTRANT -DFLTK_1_1)

# --- Include Directories ---
include_directories(
    .
    /usr/include/python3.10
    /usr/local/include/opencv4
    /usr/local/include
    /usr/X11R6/include
    /usr/lib/x86_64-linux-gnu/glib-2.0/include
    /usr/include/glib-2.0
    /usr/include/pango-1.0
    /usr/include/harfbuzz
    /usr/include/cairo
    /usr/local/include/ndi
    /usr/local/include/lunasvg
    ${CEF_PATH}
    ${CEF_PATH}/include
)

# --- Link Directories ---
link_directories(/usr/local/lib /usr/lib /usr/X11R6/lib)

# --- Shared Library: libhtml_window.so ---
if(USE_CEF)
    add_library(html_window SHARED html_window.cpp)
    target_compile_definitions(html_window PRIVATE CEF_USE_SANDBOX __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS)
    target_compile_options(html_window PRIVATE -fPIC -Wno-unused-parameter -Wno-class-memaccess)
    
    # Matching your 'g++' requirement: CMake uses CMAKE_CXX_COMPILER. 
    # If your global compiler is clang, you may need to force this specific target.
    
    target_link_libraries(html_window PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/libcef.so 
        ${CEF_PATH}/build/libcef_dll_wrapper/libcef_dll_wrapper.a
        cairo
    )
    
    # Mimic: cp libhtml_window.so ..
    add_custom_command(TARGET html_window POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:html_window> ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()

# --- Shared Library: libosg_camera.so ---
if(USE_OSG)
    add_library(osg_camera SHARED osg.cpp)
    target_compile_options(osg_camera PRIVATE -fPIC -Wno-unused-parameter)
    target_link_libraries(osg_camera PRIVATE osg osgViewer osgDB osgGA cairo)
    
    add_custom_command(TARGET osg_camera POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:osg_camera> ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()

# --- Main Executable: dvptz ---
set(COWCAM_SOURCES
    test_serial_ports.cpp
    cow_simple_pulse.c
    curl.cpp
    run_python.cpp
    irc.c
    read_wave.cpp
    extract_video.cpp
    extract_audio.cpp
    vlc_window.cpp
    muxer.cpp
    PulseAudio.cpp
    pulse_devices.cpp
    embed_app.cpp
    networking.cpp
    video_player.cpp
    dvptz.cpp
)

add_executable(dvptz ${COWCAM_SOURCES})
target_compile_definitions(dvptz PRIVATE __STDC_CONSTANT_MACROS)

# STDDYN and CVLIBS Translation
target_link_libraries(dvptz PRIVATE 
    opencv_imgproc opencv_videoio opencv_core opencv_imgcodecs opencv_highgui opencv_dnn opencv_objdetect
    avformat avcodec avutil swresample swscale avfilter gobject-2.0 pangocairo-1.0 pango-1.0 asound
    pulse pulse-simple vlc python3.10 curl uuid blend2d cairo 
    fltk fltk_images lunasvg plutovg rt jpeg Xcursor X11 xcb Xdmcp Xau Xext Xtst m 
    Xft Xrender Xfixes Xinerama Xcomposite xml2 fontconfig expat freetype fftw3 z lzma 
    visca_ip bz2 ircclient cjson magic FLAC stdc++
)

if(USE_CEF)
    add_dependencies(dvptz html_window)
    target_link_libraries(dvptz PRIVATE html_window)
endif()

if(USE_OSG)
    add_dependencies(dvptz osg_camera)
    target_link_libraries(dvptz PRIVATE osg_camera)
endif()

# --- Secondary Executable: intro ---
add_executable(intro intro.cpp)
target_link_libraries(intro PRIVATE fltk cairo)
add_custom_command(TARGET intro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:intro> ${CMAKE_CURRENT_SOURCE_DIR}/..
)

