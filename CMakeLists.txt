cmake_minimum_required(VERSION 3.12)
project(ffmpeg-bink CXX)

# This is where build.sh should build ffmpeg into
set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/FFmpeg/install/)

if (MSVC)
    message("Microsoft Compiler")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /FORCE:UNRESOLVED")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /FORCE:UNRESOLVED")
endif()

# Build a shared library to encapsulate all of FFMPEG's code
# Since we have to hack together a build with LTCG using /FORCE
# since FFMPEG uses the assumption that code in unreachable branches
# can safely reference undefined functions. In LTCG mode however,
# MSVC will not tolerate dead code elimination before resolving
# referenced symbols.
add_library(
        ffmpeg-bink
        SHARED
        videoplayer.cpp
        videoplayer.h
        videoplayer_c.cpp
        videoplayer.def
)
target_link_directories(ffmpeg-bink PUBLIC ${FFMPEG_DIR}lib/)
target_link_libraries(ffmpeg-bink PUBLIC libavcodec libavformat libavutil)
target_include_directories(ffmpeg-bink PRIVATE ${FFMPEG_DIR}include)
target_link_libraries(ffmpeg-bink PRIVATE bcrypt.lib)
set_target_properties(ffmpeg-bink PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

install(FILES $<TARGET_PDB_FILE:ffmpeg-bink> DESTINATION runtimes/${RUNTIME_ID}/native)
install(FILES $<TARGET_FILE:ffmpeg-bink> DESTINATION runtimes/${RUNTIME_ID}/native)
