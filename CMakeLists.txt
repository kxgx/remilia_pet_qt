cmake_minimum_required(VERSION 3.16)
project(RemiliaPet VERSION 1.0.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Cross-platform build type detection ──────────────────────────────
# Supported configurations:
#   cmake -B build -DVCPKG_TARGET_TRIPLET=x64-windows-static     (Windows x64, standalone)
#   cmake -B build -DVCPKG_TARGET_TRIPLET=arm64-windows-static   (Windows ARM64, standalone)
#   cmake -B build -DVCPKG_TARGET_TRIPLET=x64-windows            (Windows x64, dynamic)
#   cmake -B build                                               (macOS/Linux auto-detect)

# Detect static Qt via vcpkg triplet
if(VCPKG_TARGET_TRIPLET MATCHES "static")
    set(STATIC_BUILD ON)
endif()

if(STATIC_BUILD)
    if(MSVC)
        if(VCPKG_TARGET_TRIPLET MATCHES "static-md")
            # static-md: static libs + dynamic CRT (/MD)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
            add_link_options(/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCMTD)
        else()
            # pure static: static libs + static CRT (/MT)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
            add_link_options(/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD /NODEFAULTLIB:LIBCMTD)
        endif()
    endif()
    # Linux/macOS static: ensure static runtime if using GCC/Clang
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
endif()

# ── Qt packages ─────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ── Sources ─────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/petwindow.cpp
    src/petwindow.h
    src/audioplayer.cpp
    src/audioplayer.h
    src/inappfiledialog.cpp
    src/inappfiledialog.h
    src/resourcewindow.cpp
    src/resourcewindow.h
    src/keydisplaywindow.cpp
    src/keydisplaywindow.h
    src/keyhook.h
    resources.qrc
)

# ── Platform-specific sources & flags ───────────────────────────────
if(WIN32)
    list(APPEND SOURCES resources.rc)
    list(APPEND SOURCES src/keyhook_win.cpp)
    set(GUI_TYPE WIN32)
    if(STATIC_BUILD)
        list(APPEND SOURCES src/static_plugins.cpp)
    endif()
elseif(APPLE)
    list(APPEND SOURCES src/keyhook_mac.mm)
    set(GUI_TYPE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
else()
    find_package(X11 QUIET)
    if(X11_FOUND)
        list(APPEND SOURCES src/keyhook_linux.cpp)
    endif()
    set(GUI_TYPE "")
endif()

add_executable(RemiliaPet ${GUI_TYPE} ${SOURCES})
set_target_properties(RemiliaPet PROPERTIES OUTPUT_NAME "蕾米埃尔桌宠_Qt6")

if(APPLE)
    set_target_properties(RemiliaPet PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.remilia.pet"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    # Explicitly copy icon into bundle Resources (more reliable than SOURCES list)
    add_custom_command(TARGET RemiliaPet POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/icon.icns"
        "$<TARGET_BUNDLE_CONTENT_DIR:RemiliaPet>/Resources/icon.icns"
        COMMENT "Copying icon.icns to bundle Resources"
    )
endif()

# ── Qt module linkage ───────────────────────────────────────────────
target_link_libraries(RemiliaPet PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(X11_FOUND)
    target_link_libraries(RemiliaPet PRIVATE X11::X11)
endif()
if(APPLE)
    # 键位显示：CGEventTap 在 ApplicationServices（含 CoreGraphics）
    target_link_libraries(RemiliaPet PRIVATE "-framework ApplicationServices")
endif()

# ── Platform-specific system libraries (static builds) ──────────────
if(STATIC_BUILD)
    if(WIN32)
        target_link_libraries(RemiliaPet PRIVATE
            ws2_32 version winmm secur32 ncrypt bcrypt dxguid
        )
    elseif(APPLE)
        target_link_libraries(RemiliaPet PRIVATE
            "-framework AppKit"
            "-framework Security"
        )
    elseif(UNIX)
        target_link_libraries(RemiliaPet PRIVATE
            ${CMAKE_DL_LIBS}
        )
    endif()
endif()

# ── Build info output ───────────────────────────────────────────────
message(STATUS "Target:       ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Static build: ${STATIC_BUILD}")
if(DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
endif()

# ── Install rules ───────────────────────────────────────────────────
install(TARGETS RemiliaPet
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION .
)
