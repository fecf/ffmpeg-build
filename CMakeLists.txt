cmake_minimum_required(VERSION 3.12)
project(ffmpeg-bink CXX)

# This is where build.sh should build ffmpeg into
set(FFMPEG_DIR ${CMAKE_CURRENT_LIST_DIR}/FFmpeg/install/)

if (MSVC)
    message("Microsoft Compiler")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /FORCE:UNRESOLVED")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /FORCE:UNRESOLVED")
endif ()

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
set_property(TARGET ffmpeg-bink PROPERTY CXX_STANDARD 14)
target_link_directories(ffmpeg-bink PRIVATE ${FFMPEG_DIR}lib/)
target_include_directories(ffmpeg-bink PRIVATE ${FFMPEG_DIR}include)
set_target_properties(ffmpeg-bink PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

if (WIN32)
    target_link_libraries(ffmpeg-bink PRIVATE libavcodec libavformat libavutil libswscale)
    target_link_libraries(ffmpeg-bink PRIVATE bcrypt.lib)
    install(FILES $<TARGET_PDB_FILE:ffmpeg-bink> DESTINATION runtimes/${RUNTIME_ID}/native)
else ()
    target_link_libraries(ffmpeg-bink PRIVATE avcodec avformat avutil)
    set_target_properties(ffmpeg-bink PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif ()
install(FILES $<TARGET_FILE:ffmpeg-bink> DESTINATION runtimes/${RUNTIME_ID}/native)
