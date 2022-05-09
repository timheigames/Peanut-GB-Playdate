cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
#	execute_process(
#			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
#			COMMAND head -n 1
#			COMMAND cut -c9-
#			OUTPUT_VARIABLE SDK
#			OUTPUT_STRIP_TRAILING_WHITESPACE
#	)
	set(SDK /home/timhei/playdate) # this is just for WSL
endif()
message(SKPATH ${SDK})
if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

# Game Name Customization
set(PLAYDATE_GAME_NAME peanutGB_Playdate)
set(PLAYDATE_GAME_DEVICE peanutGB_Playdate_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(${PLAYDATE_GAME_DEVICE} ${SDK}/C_API/buildsupport/setup.c src/main.c)
	if(NOT UNIX)
		add_custom_command(TARGET ${PLAYDATE_GAME_DEVICE} PRE_BUILD COMMAND START /wait taskkill /f /im PlaydateSimulator.exe)
	endif()
	TARGET_SOURCES(${PLAYDATE_GAME_DEVICE} PRIVATE src/main.c src/rom_list.c
			src/peanut_gb.h src/minigb_apu/minigb_apu.c src/minigb_apu/minigb_apu.h)
	TARGET_INCLUDE_DIRECTORIES(${PLAYDATE_GAME_DEVICE} PRIVATE ../../)
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED src/main.c)
	if(NOT UNIX)
		add_custom_command(TARGET ${PLAYDATE_GAME_NAME} PRE_BUILD COMMAND START /wait taskkill /f /im PlaydateSimulator.exe)
	endif()
	TARGET_SOURCES(${PLAYDATE_GAME_NAME} PRIVATE src/main.c src/rom_list.c
			src/peanut_gb.h src/minigb_apu/minigb_apu.c src/minigb_apu/minigb_apu.h)
	TARGET_INCLUDE_DIRECTORIES(${PLAYDATE_GAME_NAME} PRIVATE ../../)
endif()

ADD_COMPILE_DEFINITIONS(ENABLE_SOUND)
ADD_COMPILE_DEFINITIONS(ENABLE_SOUND_MINIGB)

include(${SDK}/C_API/buildsupport/playdate_game.cmake)

