cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(grr C CXX)

option(GRR_TESTS "Enable tests build" OFF)
option(GRR_ENTT "Enable EnTT support" OFF)
option(GRR_LUA "Enable Lua support" OFF)
option(GRR_IMGUI "Enable ImGui support" OFF)
option(GRR_SERIALIZATION "Enable serialization support" OFF)
option(GRR_PREDECLARE_MEMBERS "Enable compile-time members pre-declaration" ON)

add_library(grr INTERFACE)
target_include_directories(grr INTERFACE "include")
if (GRR_ENTT)
	target_compile_definitions(grr PUBLIC "GRR_ENTT")
endif()

if (GRR_LUA)
	target_compile_definitions(grr PUBLIC "GRR_LUA")
endif()

if (GRR_IMGUI)
	target_compile_definitions(grr PUBLIC "GRR_IMGUI")
endif()

if (GRR_SERIALIZATION)
	target_compile_definitions(grr PUBLIC "GRR_SERIALIZATION")
endif()

if (GRR_PREDECLARE_MEMBERS)
	target_compile_definitions(grr PUBLIC "GRR_PREDECLARE_MEMBERS")
endif()

add_library(GRR::grr ALIAS grr)

if (GRR_TESTS)
	add_subdirectory("tests")
endif()