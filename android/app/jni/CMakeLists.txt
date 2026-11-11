# ComboBreaker for Android: SDL2 (its sources in jni/SDL, put there by
# tools/android-prepare.sh) and the program, built as libmain.so, which
# SDL's Java side loads and starts. The same sources as the desktop program,
# plus the touch controls and the Java bridge; no game code: the game runs
# on the interpreter (or code built on the phone).
cmake_minimum_required(VERSION 3.18)
project(ComboBreaker C)

add_subdirectory(SDL)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(S ${ROOT}/src)
set(CHDR ${S}/third_party/libchdr)

# <SDL2/SDL.h>, as on the desktop
set(INC ${CMAKE_CURRENT_BINARY_DIR}/inc)
file(MAKE_DIRECTORY ${INC})
if(NOT EXISTS ${INC}/SDL2)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include ${INC}/SDL2 SYMBOLIC)
endif()

# the headers that code built on the phone needs, embedded
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(HEADERS_C ${CMAKE_CURRENT_BINARY_DIR}/rec_headers.c)
add_custom_command(OUTPUT ${HEADERS_C}
    COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/embed.py ${HEADERS_C}
            rec_abi_h=${S}/rec_abi.h rec_gen_h=${S}/rec_gen.h adsp_core_h=${S}/adsp_core.h
    DEPENDS ${S}/rec_abi.h ${S}/rec_gen.h ${S}/adsp_core.h ${ROOT}/tools/embed.py)

add_library(main SHARED
    ${S}/main.c ${S}/cpu.c ${S}/disasm.c ${S}/mem.c ${S}/ide.c ${S}/dcs.c ${S}/adsp.c ${S}/adsprec.c ${S}/rec.c
    ${S}/frontend.c ${S}/config.c ${S}/osd.c ${S}/png.c ${S}/state.c ${S}/video.c ${S}/shaders.c ${S}/icon.c
    ${S}/adjust.c ${S}/recomp.c ${S}/rec_build.c ${S}/training.c ${S}/home.c ${S}/net.c ${S}/netplay.c
    ${S}/game.c ${S}/game_ki1.c ${S}/game_ki2.c ${S}/picker.c ${S}/controller_labels.c ${S}/launch.c
    ${S}/rec_none.c ${S}/touch.c ${S}/android.c
    ${S}/third_party/libchdr_all.c ${HEADERS_C})

target_include_directories(main PRIVATE ${INC} ${INC}/SDL2 ${S}
    ${CHDR}/include ${CHDR}/deps/lzma-26.02/include ${CHDR}/deps/miniz-3.1.2 ${CHDR}/deps/zstd-1.5.7)
target_compile_options(main PRIVATE -O2 -fno-strict-aliasing -Wno-unused-parameter)
set_source_files_properties(${S}/third_party/libchdr_all.c PROPERTIES COMPILE_OPTIONS "-w")
target_link_libraries(main SDL2 m dl log android)
