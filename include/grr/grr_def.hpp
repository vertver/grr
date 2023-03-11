/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_DEF_HPP_INCLUDED
#define GRR_DEF_HPP_INCLUDED

#ifndef GRR_TYPE_ID
#define GRR_TYPE_ID std::uint64_t
#endif

#ifndef GRR_INVALID_SIZE
#define GRR_INVALID_SIZE static_cast<std::size_t>(-1)
#endif

#ifndef GRR_INVALID_ID
#define GRR_INVALID_ID static_cast<GRR_TYPE_ID>(-1)
#endif

#ifndef GRR_DISABLE_EXCEPTIONS
#include <exception>
#endif

#ifndef GRR_VECTOR
#include <vector>
#define GRR_VECTOR std::vector
#endif

#ifndef GRR_HASH_MAP
#include <unordered_map>
#define GRR_HASH_MAP std::unordered_map
#endif

#ifndef GRR_HASH_SET
#include <unordered_set>
#define GRR_HASH_SET std::unordered_set
#endif

#ifndef GRR_STRING
#include <string>
#define GRR_STRING std::string
#endif

#ifndef GRR_STRING_VIEW
#include <string_view>
#define GRR_STRING_VIEW std::string_view
#endif

#ifdef _MSC_VER
#ifndef GRR_WIDESTRING
#include <string>
#define GRR_WIDESTRING std::wstring
#endif

#ifndef GRR_WIDESTRING_VIEW
#include <string_view>
#define GRR_WIDESTRING_VIEW std::wstring_view
#endif
#endif
#else
#define GRR_WIDESTRING
#define GRR_WIDESTRING_VIEW
#endif