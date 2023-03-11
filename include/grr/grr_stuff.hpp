/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_STUFF_HPP_INCLUDED
#define GRR_STUFF_HPP_INCLUDED

#include <typeinfo>
#include <system_error>
#include <cstdint>
#include <utility>

#ifdef GRR_PREDECLARE_FIELDS
#ifndef PFR_HPP
#include <pfr.hpp>
#endif

#ifndef VISIT_STRUCT_HPP_INCLUDED
#include <visit_struct/visit_struct.hpp>
#endif

#ifndef VISIT_STRUCT_INTRUSIVE_HPP_INCLUDED
#include <visit_struct/visit_struct_intrusive.hpp>
#endif
#endif

#if defined(GRR_TS_REFLECT) && !defined(__cpp_lib_reflection)
#error Unsupported compile for C++ reflection feature
#endif

#ifndef GRR_USER_TYPES
#define GRR_USER_TYPES
#endif

#if defined(_MSC_VER) && __cplusplus == 199711L
#define GRR_CXX _MSVC_LANG 
#else
#define GRR_CXX __cplusplus 
#endif

#if GRR_CXX >= 202002L
#define GRR_CXX20_SUPPORT 1
#elif GRR_CXX < 201703L
#error Incompatible version of C++
#endif

#ifndef GRR_CONSTEXPR
#ifdef GRR_CXX20_SUPPORT
#define GRR_CONSTEXPR constexpr
#else
#define GRR_CONSTEXPR
#endif
#endif

#ifndef GRR_TYPE_NAME
#ifdef _MSC_VER
#define GRR_TYPE_NAME __FUNCSIG__
#else
#define GRR_TYPE_NAME __PRETTY_FUNCTION__
#endif
#endif

#ifndef GRR_PLATFORM_NAME
#ifdef _WIN32
#define GRR_PLATFORM_NAME "win32-";
#elif __APPLE__ || __MACH__
##define GRR_PLATFORM_NAME "macos-";
#elif __linux__
##define GRR_PLATFORM_NAME "linux-";
#elif __FreeBSD__
##define GRR_PLATFORM_NAME "freebsd-";
#elif __unix || __unix__
##define GRR_PLATFORM_NAME "unix-";
#endif
#endif

#ifndef GRR_COMPILER_NAME
#if defined(__clang__)
#define GRR_COMPILER_NAME "clang";
#elif defined(__GNUC__)
#define GRR_COMPILER_NAME "gcc";
#elif defined(_MSC_VER)
#define GRR_COMPILER_NAME "msvc";
#endif
#endif

#endif