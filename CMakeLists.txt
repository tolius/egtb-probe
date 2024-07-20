cmake_minimum_required(VERSION 3.16.0)

if (WIN32)
	set(ARCH_FLAGS "AVX2")
else()
	set(ARCH_FLAGS "native")
endif()

add_subdirectory(src/tb/egtb/dictzip)

project(egtb_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ARCH_FLAGS)
	if (WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch:${ARCH_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCH_FLAGS}")
	endif()
endif()

set(TB_SRC 
	src/tb/benchmark.cpp
	src/tb/bitbase.cpp
	src/tb/bitboard.cpp
	src/tb/bitboard.h
	src/tb/endgame.cpp
	src/tb/endgame.h
	src/tb/evaluate.cpp
	src/tb/evaluate.h
	src/tb/material.cpp
	src/tb/material.h
	src/tb/misc.cpp
	src/tb/misc.h
	src/tb/movegen.cpp
	src/tb/movegen.h
	src/tb/movepick.cpp
	src/tb/movepick.h
	src/tb/pawns.cpp
	src/tb/pawns.h
	src/tb/psqt.cpp
	src/tb/position.cpp
	src/tb/position.h
	src/tb/search.cpp
	src/tb/search.h
	src/tb/thread.cpp
	src/tb/thread.h
	src/tb/thread_win32_osx.h
	src/tb/timeman.cpp
	src/tb/timeman.h
	src/tb/tt.cpp
	src/tb/tt.h
	src/tb/tune.cpp
	src/tb/tune.h
	src/tb/types.h
	src/tb/uci.cpp
	src/tb/uci.h
	src/tb/ucioption.cpp
	#
	src/tb/syzygy/tbprobe.cpp
	src/tb/syzygy/tbprobe.h
	src/tb/incbin/incbin.h
)
source_group("src" FILES ${TB_SRC})

set(EGTB_SRC 
	src/tb/egtb/elements.cpp
	src/tb/egtb/elements.h
	src/tb/egtb/tb_reader.cpp
	src/tb/egtb/tb_reader.h
	src/tb/egtb/tb_idx.cpp
	src/tb/egtb/tb_idx.h
)
source_group("EGTB" FILES ${EGTB_SRC})

add_executable(${PROJECT_NAME}
	${TB_SRC}
	${EGTB_SRC}
	src/main.cpp
	src/egtb.h
	src/egtb.cpp
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE ANTI
	PRIVATE USE_POPCNT
	PRIVATE USE_PEXT
)

if (WIN32)
	target_link_libraries(${PROJECT_NAME}
		$<TARGET_OBJECTS:dictzip>
	)
	target_compile_options(${PROJECT_NAME}
		PRIVATE "/Ox"
		PRIVATE "/GL"
	)
else()
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE NDEBUG
	)
	find_package(Threads REQUIRED)
	target_link_libraries(${PROJECT_NAME}
		$<TARGET_OBJECTS:dictzip>
		Threads::Threads
		-lstdc++fs
	)
	target_compile_options(${PROJECT_NAME}
		PRIVATE "-Ofast"
		PRIVATE "-flto"
		PRIVATE "-march=${ARCH_FLAGS}"
	)
endif()
