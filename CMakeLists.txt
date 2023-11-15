cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(grr C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GRR_TESTS "Enable tests build" OFF)
option(GRR_SAMPLES "Enable samples build" ON)
option(GRR_SERIALIZATION "Enable serialization support" OFF)
option(GRR_PREDECLARE_FIELDS "Enable compile-time members pre-declaration" ON)
option(GRR_TS_REFLECT "Enable experimental reflection TS feature" OFF) # not supported yet	
option(GRR_ENTT "Enable EnTT support" OFF)
option(GRR_LUA "Enable Lua support" OFF)
option(GRR_IMGUI "Enable ImGui support" OFF)

add_library(grr INTERFACE)
target_include_directories(grr INTERFACE "include")
target_compile_features(grr INTERFACE cxx_std_20)

if (GRR_SERIALIZATION)
	target_compile_definitions(grr INTERFACE "GRR_SERIALIZATION")
	message(STATUS "[GRR]: serialization support enabled.")
endif()

if (GRR_PREDECLARE_FIELDS)
	target_compile_definitions(grr INTERFACE "GRR_PREDECLARE_FIELDS")
	message(STATUS "[GRR]: fields pre-declaration enabled.")
endif()

if (GRR_TS_REFLECT)
	target_compile_definitions(grr INTERFACE "GRR_TS_REFLECT")
	message(STATUS "[GRR]: TS Reflect support enabled.")
endif()

if (GRR_ENTT)
	target_compile_definitions(grr INTERFACE "GRR_ENTT")
	message(STATUS "[GRR]: EnTT support enabled.")
endif()

if (GRR_LUA)
	target_compile_definitions(grr INTERFACE "GRR_LUA")
	message(STATUS "[GRR]: Lua support enabled.")
endif()

if (GRR_IMGUI)
	target_compile_definitions(grr INTERFACE "GRR_IMGUI")
	message(STATUS "[GRR]: ImGui support enabled.")
endif()

add_library(GRR::grr ALIAS grr)

if (GRR_TESTS)
	add_subdirectory("tests")
endif()

if (GRR_SAMPLES)
	add_subdirectory("samples")
endif()