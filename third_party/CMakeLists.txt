# All third party libraries
cmake_minimum_required(VERSION 3.4)

#nanovg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/nanovg/src)
add_library(nanovg nanovg/src/nanovg.c)
target_include_directories(nanovg PUBLIC nanovg/src)

add_subdirectory(lua-5.3.5)

if(WIN32)
	# Use bundled third party libraries on windows
	add_subdirectory(freetype)
	add_subdirectory(jpeg-9b)
	add_subdirectory(zlib-1.2.8)
	add_subdirectory(SDL2-2.0.4)
	add_subdirectory(lpng1621)
	add_subdirectory(libogg-1.3.2)
	add_subdirectory(libvorbis-1.3.5)
endif(WIN32)
