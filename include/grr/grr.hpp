/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_HPP_INCLUDED
#define GRR_HPP_INCLUDED
#include <grr/grr_stuff.hpp>
#include <grr/grr_def.hpp>
#include <grr/grr_types.hpp>
#include <grr/grr_base.hpp>
#include <grr/grr_serialization.hpp>
#include <grr/grr_entt.hpp>
#include <grr/grr_lua.hpp>

#ifndef VISITABLE_STRUCT
#define GRR_REFLECT(__VA_ARGS__) 
#else
#define GRR_REFLECT VISITABLE_STRUCT
#endif
#endif