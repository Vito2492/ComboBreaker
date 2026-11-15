# ComboBreaker for iPhone and iPad: SDL2 (its sources in SDL_DIR, built
# static) and the same program as on the desktop, plus the touch controls.
# No game code, and iOS can't build or load any on the phone, so the game
# runs on the interpreter. Built unsigned; the player's sideloading tool
# (SideStore, Sideloadly, AltStore) signs it with their own Apple ID.
#   cmake -S ios -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DSDL_DIR=path
cmake_minimum_required(VERSION 3.20)
project(ComboBreaker C)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${SDL_DIR} sdl EXCLUDE_FROM_ALL)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(S ${ROOT}/src)
set(CHDR ${S}/third_party/libchdr)

# <SDL2/SDL.h>, as on the desktop
set(INC ${CMAKE_CURRENT_BINARY_DIR}/inc)
file(MAKE_DIRECTORY ${INC})
if(NOT EXISTS ${INC}/SDL2)
    file(CREATE_LINK ${SDL_DIR}/include ${INC}/SDL2 SYMBOLIC)
endif()

# the headers translated code needs, embedded (unused here, but linked in)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(HEADERS_C ${CMAKE_CURRENT_BINARY_DIR}/rec_headers.c)
add_custom_command(OUTPUT ${HEADERS_C}
    COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/embed.py ${HEADERS_C}
            rec_abi_h=${S}/rec_abi.h rec_gen_h=${S}/rec_gen.h adsp_core_h=${S}/adsp_core.h
    DEPENDS ${S}/rec_abi.h ${S}/rec_gen.h ${S}/adsp_core.h ${ROOT}/tools/embed.py)

# the icon, drawn by the desktop program (tools/ios-prepare.sh)
set(ICONS ${CMAKE_CURRENT_SOURCE_DIR}/icons/AppIcon60x60@2x.png ${CMAKE_CURRENT_SOURCE_DIR}/icons/AppIcon60x60@3x.png
          ${CMAKE_CURRENT_SOURCE_DIR}/icons/AppIcon76x76@2x~ipad.png)

add_executable(ComboBreaker MACOSX_BUNDLE
    ${S}/main.c ${S}/cpu.c ${S}/disasm.c ${S}/mem.c ${S}/ide.c ${S}/dcs.c ${S}/adsp.c ${S}/adsprec.c ${S}/rec.c
    ${S}/frontend.c ${S}/config.c ${S}/osd.c ${S}/png.c ${S}/state.c ${S}/video.c ${S}/shaders.c ${S}/icon.c
    ${S}/adjust.c ${S}/recomp.c ${S}/rec_build.c ${S}/training.c ${S}/home.c ${S}/net.c ${S}/netplay.c
    ${S}/game.c ${S}/game_ki1.c ${S}/game_ki2.c ${S}/picker.c ${S}/controller_labels.c ${S}/launch.c
    ${S}/rec_none.c ${S}/touch.c
    ${S}/third_party/libchdr_all.c ${HEADERS_C} ${ICONS})

target_include_directories(ComboBreaker PRIVATE ${INC} ${INC}/SDL2 ${S}
    ${CHDR}/include ${CHDR}/deps/lzma-26.02/include ${CHDR}/deps/miniz-3.1.2 ${CHDR}/deps/zstd-1.5.7)
target_compile_options(ComboBreaker PRIVATE -O2 -fno-strict-aliasing -Wno-unused-parameter)
set_source_files_properties(${S}/third_party/libchdr_all.c PROPERTIES COMPILE_OPTIONS "-w")
set_source_files_properties(${ICONS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
target_link_libraries(ComboBreaker PRIVATE SDL2::SDL2main SDL2::SDL2-static m)

set(VERSION "1.0" CACHE STRING "version shown by iOS")
set(BUILD_NUMBER "1" CACHE STRING "build number")
set_target_properties(ComboBreaker PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${BUILD_NUMBER}
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER io.github.vito2492.combobreaker
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 14.0
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
