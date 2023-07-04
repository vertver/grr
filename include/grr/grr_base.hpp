/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* MIT License
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED
#include <grr/detail/name_parser.hpp>

namespace grr
{
    enum class errors : int
    {
        invalid_argument,
        unregistered_id,
        already_registered,
        parsing_failed,
        out_of_range
    };

    struct error_category : public std::error_category
    {
        std::string message(int c) const
        {
            static const char* err_msg[] =
            {
                "Invalid argument",
                "Unregisted ID",
                "Already registered",
                "Parsing failed",
                "Out of range"
            };

            return err_msg[c];
        }

        const char* name() const noexcept { return "GRR Error code"; }
        const static error_category& get()
        {
            const static error_category category_const;
            return category_const;
        }
    };

    static inline std::error_code make_error_code(errors e)
    {
        return std::error_code(static_cast<int>(e), error_category::get());
    }

    struct field
    {
        GRR_CONSTEXPR field(const char* new_name, type_id new_id, std::size_t new_offset)
            : name(new_name), id(new_id), offset(new_offset) {}

        GRR_CONSTEXPR field(const string_view& new_name, type_id new_id, std::size_t new_offset)
            : name(new_name), id(new_id), offset(new_offset) {}

        field() = default;
        field(const field&) = default;
        field(field&&) = default;
        field& operator=(const field&) = default;
        field& operator=(field&&) = default;

        std::size_t offset;
        type_id id;
        string name;
    };

    struct type_context
    {
        type_id base_type;
        std::size_t size;
        string name;
        vector<field> fields;
    };

    class context
    {
    private:
        using storage_map = hash_map<type_id, type_context>;
        hash_map<type_id, type_context> storage;

    public:
        type_context& at(type_id id)
        {
            return storage.at(id);
        }

        const type_context& at(type_id id) const
        {
            return storage.at(id);
        }

        bool contains(type_id id) const
        {
#ifdef GRR_CXX20
            return storage.contains(id);
#else
            return storage.find(id) != storage.end();
#endif
        }

        std::size_t size(type_id id) const
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                return static_cast<std::size_t>(-1);
            }

            return it->second.size;
        }

        const type_context& obtain(type_id id) const
        {
            return storage.at(id);
        }

        storage_map::const_iterator begin() const
        {
            return storage.begin();
        }

        storage_map::const_iterator end() const
        {
            return storage.end();
        }

        void rename(type_id id, const char* new_name, std::error_code& err)
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                err = make_error_code(errors::unregistered_id);
                return;
            }

            it->second.name = new_name;
        }

        void rename(type_id id, const string_view& new_name, std::error_code& err)
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                err = make_error_code(errors::unregistered_id);
                return;
            }

            it->second.name = string(new_name.begin(), new_name.end());
        }

        void add(type_id id, const type_context& type)
        {
            storage.emplace(std::make_pair(id, type));
        }

        void add(type_id id, type_context&& type)
        {
            storage.emplace(std::make_pair(id, type));
        }
    };

    template<typename T>
    static constexpr auto type_name()
    {
        return grr::detail::compiler_type_name<T>(0);
    }

    static inline const char* type_name(const context& ctx, type_id id)
    {
        if (!ctx.contains(id)) {
            return "";
        }

        return ctx.at(id).name.c_str();
    }

    template<typename T>
    static constexpr T binhash(const std::string_view& str)
    {
        T hash = T(5381);
        for (const char& sym : str) {
            hash *= 0x21;
            hash += sym;
        }

        return hash;
    }

    template<typename T>
    static constexpr T binhash(const char* str, std::size_t size)
    {
        T hash = T(5381);
        for (std::size_t i = 0; i < size; i++) {
            hash *= 0x21;
            hash += str[i];
        }

        return hash;
    }

    template<typename T>
    static constexpr T binhash(const char* str)
    {
        return *str != '\0' ? static_cast<unsigned int>(*str) + 33 * binhash<T>(str + 1) : 5381;
    }

    template<typename T>
    static constexpr T optimized_serializable_hash(const grr::string_view& str)
    {
        constexpr std::pair<grr::string_view, std::size_t> strings[] =
        {
            { string_view("struct")				    , grr::binhash<std::size_t>("struct")					},  // MSVC stuff
            { string_view("`anonymous-namespace'::"), grr::binhash<std::size_t>("`anonymous-namespace'::")	},  // MSVC stuff
            { string_view("class")					, grr::binhash<std::size_t>("class")				    },  // MSVC stuff

            { string_view("(anonymous namespace)::"), grr::binhash<std::size_t>("(anonymous namespace)::")	},  // Clang stuff

            { string_view("{anonymous}::") 			, grr::binhash<std::size_t>("{anonymous}::")			},  // GCC stuff
            { string_view("__cxx11::") 				, grr::binhash<std::size_t>("__cxx11::")				},  // GCC stuff
            { string_view("__cxx14::") 				, grr::binhash<std::size_t>("__cxx14::")				},  // GCC stuff
            { string_view("__cxx17::") 				, grr::binhash<std::size_t>("__cxx17::")				},  // GCC stuff
            { string_view("__cxx20::") 				, grr::binhash<std::size_t>("__cxx20::")				},  // GCC stuff
            { string_view("__cxx23::") 				, grr::binhash<std::size_t>("__cxx23::")				}   // GCC stuff
        };

        // 'c' - 0x63 - b01100011
        // 's' - 0x73 - b01110011
        // '`' - 0x60 - b01100000
        // ' ' - 0x20 - b00100000
        
        // '(' - 0x28 - b00101000
        
        // '{' - 0x7B - b01111011
        // '_' - 0x5F - b01011111
        // ' ' - 0x20 - b00100000

        T hash = T(5381);

#ifdef _MSC_VER
        for (size_t i = 0; i < str.size(); i++)
        {
            char sym = str[i];
            std::uint8_t flag = (sym & 0b00101100);

            //GRR_LIKELY
            if (flag != 0b01100000) {
                hash *= 0x21;
                hash += str[i];
                continue;
            }

            if (sym == 'c' && i + 5 < str.size()) {
                constexpr std::size_t class_hash = grr::binhash<std::size_t>("class");
                std::size_t hash = grr::binhash(&str[i], 5);

                //GRR_LIKELY
                if (hash == class_hash) {
                    i += 5;
                    continue;
                }

                hash *= 0x21;
                hash += str[i];
                continue;
            }

            if (sym == 's' && i + 6 < str.size()) {
                constexpr std::size_t struct_hash = grr::binhash<std::size_t>("struct");
                std::size_t hash = grr::binhash(&str[i], 6);

                //GRR_LIKELY
                if (hash == struct_hash) {
                    i += 6;
                    continue;
                }

                hash *= 0x21;
                hash += str[i];
                continue;
            }

            /*
            // 'c' or 's'
            if ((sym & 0b01101011) == 0b01100011) {

            }
            */
        }
#endif

        return {};
    }

    // TODO: This method was created for compile-time only execution. It may be useful
    // to create more faster runtime implementation of this (without using std::array or
    // other dynamically allocated memory)
    template<typename T, std::size_t max_indexes = 64>
    static constexpr T serializable_hash(const grr::string_view& str)
    {
        constexpr grr::string_view strings[] =
        {
            "struct ",						// MSVC stuff
            "class ",						// MSVC stuff
            "__cxx11::",					// GCC stuff
            "__cxx14::",					// GCC stuff
            "__cxx17::",					// GCC stuff
            "__cxx20::",					// GCC stuff
            "__cxx23::",					// GCC stuff
            "{anonymous}::",				// GCC stuff
            "(anonymous namespace)::",		// Clang stuff
            "`anonymous-namespace'::"		// MSVC stuff
        };

        //  " "								// MSVC and GCC stuff

        T hash = T(5381);
        std::size_t indexes_count = 0;
        std::array<std::size_t, max_indexes> indexes;
        for (const grr::string_view& comparable : strings) {
            std::size_t start = static_cast<std::size_t>(-1);
            for (size_t i = 0; i < str.size(); i++) {
                if (start == std::size_t(-1)) {
                    if (str.at(i) == comparable.at(0)) {
                        start = i;
                        continue;
                    }
                } else {
                    const size_t offset = i - start;
                    if (offset >= comparable.size()) {
                        indexes[indexes_count] = start;
                        indexes[indexes_count + 1] = start + comparable.size();
                        indexes_count += 2;
                        if (indexes_count == indexes.size()) {
                            break;
                        }

                        start = std::size_t(-1);
                        continue;
                    }

                    if (str.at(i) != comparable.at(offset)) {
                        start = std::size_t(-1);
                        continue;
                    }
                }
            }
        }

        std::size_t current_idx = 0;
        std::sort(indexes.begin(), indexes.begin() + indexes_count);
        for (size_t i = 0; i < str.size(); i++) {
            if (current_idx > 0) {
                if (current_idx >= indexes_count) {
                    break;
                }

                if (i >= indexes[current_idx + 1]) {
                    current_idx += 2;
                }

                if (current_idx >= indexes_count) {
                    break;
                }

                if (i >= indexes[current_idx] && i < indexes[current_idx + 1]) {
                    continue;
                }
            }

            hash *= 0x21;
            hash += str[i];
        }

        return hash;
    }

    static constexpr type_id obtain_id(const char* name)
    {
        return serializable_hash<type_id>(name);
    }

    static constexpr type_id obtain_id(const grr::string_view& name)
    {
        return serializable_hash<type_id>(name);
    }

    template<typename T>
    static constexpr type_id obtain_id()
    {
        return obtain_id(type_name<grr::clean_type<T>>());
    }

    static inline std::size_t size(const context& ctx, type_id id)
    {
        return ctx.size(id);
    }

    template<typename T>
    static inline bool contains(const context& ctx)
    {
        constexpr type_id id = obtain_id<grr::clean_type<T>>();
        return ctx.contains(id);
    }

    static inline bool contains(const context& ctx, type_id id)
    {
        return ctx.contains(id);
    }

    static inline void rename(context& ctx, type_id id, std::size_t field_idx, const char* new_name, std::error_code& err)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id);
            return;
        }

        auto& fields = ctx.at(id).fields;
        if (field_idx >= fields.size()) {
            err = make_error_code(errors::invalid_argument);
            return;
        }

        fields.at(field_idx).name = new_name;
    }

    static inline void rename(context& ctx, type_id id, std::size_t field_idx, const string_view& new_name, std::error_code& err)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id);
            return;
        }

        auto& fields = ctx.at(id).fields;
        if (field_idx >= fields.size()) {
            err = make_error_code(errors::invalid_argument);
            return;
        }

        fields.at(field_idx).name = string(new_name.begin(), new_name.end());
    }

    static inline void rename(context& ctx, type_id id, const char* new_name, std::error_code& err)
    {
        ctx.rename(id, new_name, err);
    }

    static inline void rename(context& ctx, type_id id, const string_view& new_name, std::error_code& err)
    {
        ctx.rename(id, new_name, err);
    }

    static inline std::size_t offset(const context& ctx, type_id id, std::size_t field_idx, std::error_code& err)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id);
            return 0;
        }

        auto& fields = ctx.at(id).fields;
        if (field_idx >= fields.size()) {
            err = make_error_code(errors::invalid_argument);
            return 0;
        }

        return fields.at(field_idx).offset;
    }

    static inline type_id base_type(context& ctx, type_id id)
    {
        return ctx.at(id).base_type;
    }

    template<typename T>
    static constexpr type_id base_type()
    {
        return obtain_id<std::remove_pointer_t<grr::clean_type<T>>>();
    }

    template<typename T>
    static inline void rename(context& ctx, std::size_t field_idx, const string_view& new_name, std::error_code& err)
    {
        constexpr type_id id = obtain_id<grr::clean_type<T>>();
        rename(ctx, id, field_idx, new_name, err);
    }

    template<typename T>
    static inline void rename(context& ctx, std::size_t field_idx, const char* new_name, std::error_code& err)
    {
        constexpr type_id id = obtain_id<grr::clean_type<T>>();
        rename(ctx, id, field_idx, new_name, err);
    }

    template<typename T>
    static inline void rename(context& ctx, const string_view& new_name, std::error_code& err)
    {
        ctx.rename(obtain_id<grr::clean_type<T>>(), new_name, err);
    }

    template<typename T>
    static inline void rename(context& ctx, const char* new_name, std::error_code& err)
    {
        ctx.rename(obtain_id<grr::clean_type<T>>(), new_name, err);
    }

    template<typename T>
    static inline bool size(const context& ctx)
    {
        return ctx.size(obtain_id<grr::clean_type<T>>());
    }

    template<typename T>
    static inline std::size_t offset(const context& ctx, std::size_t field_idx, std::error_code& err)
    {
        constexpr type_id id = obtain_id<grr::clean_type<T>>();
        return offset(ctx, id, field_idx, err);
    }

    template<typename... Types>
    static constexpr void visit(const context& ctx, type_id id, auto&& func)
    {
        constexpr std::size_t types_count = sizeof...(Types);
    }

    namespace detail
    {
        template<typename T, typename... Args>
        static inline void construct(T* memory_to_construct, Args... args)
        {
        }

        template<typename T, typename... Args>
        static inline void destruct(T* memory_to_construct)
        {
            memory_to_construct->~T();
        }

        template<typename T>
        static constexpr void visit_static(auto data, type_id id, const char* name, bool& called, auto&& func)
        {
            using CleanDataType = std::remove_pointer_t<decltype(data)>;
            auto call_function = [](auto&& func, auto&& argument, const char* name) -> bool {
                constexpr bool callable_1 = std::is_invocable_r_v<bool, decltype(func), decltype(argument)>;
                constexpr bool callable_2 = std::is_invocable_r_v<bool, decltype(func), decltype(argument), const char*>;
                constexpr bool callable_3 = std::is_invocable_r_v<void, decltype(func), decltype(argument)>;
                constexpr bool callable_4 = std::is_invocable_r_v<void, decltype(func), decltype(argument), const char*>;
                static_assert(callable_1 || callable_2 || callable_3 || callable_4, "Captured function is not acceptable");

                if constexpr (callable_1) {
                    return func(argument);
                } else if constexpr (callable_2) {
                    return func(argument, name);
                } else if constexpr (callable_3) {
                    func(argument);
                    return true;
                } else if constexpr (callable_4) {
                    func(argument, name);
                    return true;
                }

                return false;
            };

            constexpr type_id current_id = grr::obtain_id<T>();
            if (!called && current_id == id) {
                if constexpr (std::is_const_v<CleanDataType>) {
                    called = call_function(func, *reinterpret_cast<const T*>(data));
                } else {
                    called = call_function(func, *reinterpret_cast<T*>(data));
                }
            }

            /*
            if constexpr (!std::is_same_v<T, void>) {
                constexpr type_id current_id = grr::obtain_id<T>();
                if (!called && current_id == id) {
                    if constexpr (std::is_const_v<CleanDataType>) {
                        call_function(func, *reinterpret_cast<const T*>(data));
                    } else {
                        call_function(func, *reinterpret_cast<T*>(data));
                    }

                    called = true;
                }
            }

            constexpr type_id current_ptr_id = grr::obtain_id<T*>();
            if (!called && current_ptr_id == id) {
                if constexpr (std::is_const_v<CleanDataType>) {
                    call_function(func, *reinterpret_cast<const T**>(reinterpret_cast<size_t>(data)));
                } else {
                    call_function(func, *reinterpret_cast<T**>(reinterpret_cast<size_t>(data)));
                }

                called = true;
            }
            */
        }

        template<typename... Types>
        static constexpr bool visit_static(auto data, const char* name, type_id id, auto&& func)
        {
            bool called = false;
            (visit_static<Types>(data, id, name, called, func), ...);
            return called;
        }

        template<typename... Types>
        static constexpr bool visit_static(auto data, type_id id, auto&& func)
        {
            bool called = false;
            (visit_static<Types>(data, id, "var", called, func), ...);
            return called;
        }

        template<std::size_t recursion_level = 0>
        static inline void visit(const grr::context& ctx, auto data, type_id id, std::error_code& err, auto&& func)
        {
            // [](auto& field, const char* name)
            auto call_function = [](auto&& func, auto ptr, auto id, auto size) -> bool {
                auto pair = std::make_pair(size, std::make_pair(ptr, id));
                constexpr bool callable_1 = std::is_invocable_r_v<bool, decltype(func), decltype(pair)>;
                constexpr bool callable_2 = std::is_invocable_r_v<bool, decltype(func), decltype(pair), const char*>;
                constexpr bool callable_3 = std::is_invocable_r_v<void, decltype(func), decltype(pair)>;
                constexpr bool callable_4 = std::is_invocable_r_v<void, decltype(func), decltype(pair), const char*>;
                static_assert(callable_1 || callable_2 || callable_3 || callable_4, "Captured function is not acceptable");

                if constexpr (callable_1) {
                    return func(pair);
                } else if constexpr (callable_2) {
                    return func(pair, "var0");
                } else if constexpr (callable_3) {
                    func(pair);
                    return true;
                } else if constexpr (callable_4) {
                    func(pair, "var0");
                    return true;
                }
                
                return false;
            };

            const auto& type_info = ctx.obtain(id);
            if (type_info.fields.empty()) {
                auto type_ptr = data;
                if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
                    type_ptr = static_cast<const char*>(data);
                } else {
                    type_ptr = static_cast<char*>(data);
                }

                if (!detail::visit_static<GRR_TYPES>(type_ptr, id, func)) {
                    return;
                }

                if (!ctx.contains(id)) {
                    err = make_error_code(errors::unregistered_id);
                    return;
                }
    
                if (!call_function(func, type_ptr, id, type_info.size)) {
                    err = make_error_code(errors::invalid_argument);
                    return;
                }
            } else {
                for (const auto& cfield : type_info.fields) {
                    auto field_ptr = data;
                    const type_id field_id = cfield.id;
                    if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
                        field_ptr = static_cast<const char*>(data) + cfield.offset;
                    } else {
                        field_ptr = static_cast<char*>(data) + cfield.offset;
                    }

                    if constexpr (recursion_level > 0) {
                        visit<recursion_level - 1>(ctx, field_ptr, field_id);
                    } else {
                        if (detail::visit_static<GRR_TYPES>(field_ptr, cfield.name.data(), cfield.id, func)) {
                            continue;
                        }

                        if (!ctx.contains(field_id)) {
                            err = make_error_code(errors::unregistered_id);
                            return;
                        }

                        auto& field_type = ctx.obtain(field_id);
                        if (!call_function(func, field_ptr, field_id, field_type.size)) {
                            err = make_error_code(errors::invalid_argument);
                            return;
                        }
                    }
                }
            }
        }

        /*
        template<std::size_t recursion_level = 0>
        static inline void visit(const grr::context& ctx, auto data, type_id id, const char* field_name, std::error_code& err, auto&& func)
        {
            const auto& type_info = ctx.obtain(id);
            if (type_info.fields.empty()) {
                err = make_error_code(errors::invalid_argument);
                return;
            }

            const auto name_hash = grr::binhash<grr::type_id>(field_name);
            for (const auto& cfield : type_info.fields) {
                const auto field_name_hash = grr::binhash<grr::type_id>(cfield.name.c_str());
                if (field_name_hash == name_hash) {
                    auto type_ptr = data;
                    if constexpr (std::is_const_v<std::remove_pointer_t<decltype(data)>>) {
                        type_ptr = static_cast<const char*>(data) + cfield.offset;
                    } else {
                        type_ptr = static_cast<char*>(data) + cfield.offset;
                    }

                    if (detail::visit_static<GRR_TYPES>(type_ptr, id, func)) {
                        continue;
                    }
                }
            }

            err = make_error_code(errors::out_of_range);
        }
        */
    }

    template<std::size_t recursion_level = 0>
    static inline void visit(const grr::context& ctx, auto data, type_id id, std::error_code& err, auto&& func)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id);
            return;
        }

        grr::detail::visit<recursion_level>(ctx, data, id, err, func);
        if (err) {
            return;
        }
    }

    template<typename T, std::size_t recursion_level = 0>
    static inline void visit(const grr::context& ctx, T& data, std::error_code& err, auto&& func)
    {
        constexpr type_id id = grr::obtain_id<T>();
        if constexpr (std::is_const_v<T>) {
            grr::visit<recursion_level>(ctx, reinterpret_cast<const void*>(&data), id, err, func);
        } else {
            grr::visit<recursion_level>(ctx, reinterpret_cast<void*>(&data), id, err, func);
        }
    }

    static inline void construct(const grr::context& ctx, void* memory_to_construct, type_id id, std::error_code& err)
    {
        grr::visit(ctx, memory_to_construct, id, err, [](auto& field, const char* name) {
            using CleanType = grr::clean_type<decltype(field)>;
            if constexpr (!grr::is_fallback_type_v<CleanType>) {
                grr::detail::construct<CleanType>(&field);
            }
        });
    }

    static inline void destruct(const grr::context& ctx, void* memory_to_destruct, type_id id, std::error_code& err)
    {
        grr::visit(ctx, memory_to_destruct, id, err, [](auto& field, const char* name) {
            using CleanType = grr::clean_type<decltype(field)>;
            if constexpr (!grr::is_fallback_type_v<CleanType>) {
                grr::detail::destruct<CleanType>(&field);
            }
        });
    }
        
    struct type_declaration
    {
        bool aggregate = false;
        const context* ctx;
        std::int64_t index;
        std::size_t size;
        string name;
        type_id id;
        vector<field> fields;

        GRR_CONSTEXPR type_declaration() = delete;
        GRR_CONSTEXPR type_declaration(type_declaration&) = delete;
        GRR_CONSTEXPR type_declaration(const type_declaration&) = delete;
        GRR_CONSTEXPR type_declaration(type_declaration&&) = default;

        GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const char* type_name) noexcept
            : ctx(&in_context), name(type_name), id(in_id), size(0), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const string_view& type_name) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(in_id), size(0), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const char* type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name), id(in_id), size(new_size), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, type_id in_id, const string_view& type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(in_id), size(new_size), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, const char* type_name) noexcept
            : ctx(&in_context), name(type_name), id(obtain_id(type_name)), size(0), index(-1) {}
        
        GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(0), index(-1) {}
        
        GRR_CONSTEXPR type_declaration(const context& in_context, const char* type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name), id(obtain_id(type_name)), size(new_size), index(-1) {}
        
        GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(new_size), index(-1) {}

        bool field_erase(const char* field_name)
        {
            const auto field_hash = binhash<std::uint32_t>(field_name);
            for (auto it = fields.begin(); it != fields.end(); it++) {
                const auto& field_elem = *it;
                if (field_hash == binhash<std::uint32_t>(field_elem.name.data())) {
                    fields.erase(it);
                    return true;
                }
            }

            return false;
        }

        bool field_erase(std::size_t idx)
        {
            if (idx >= fields.size()) {
                return false;
            }

            fields.erase(fields.begin() + idx);
            return true;
        }

        template<typename T>
        void emplace(const char* field_name, std::error_code& err)
        {
            constexpr type_id current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id);
                return;
            }

            const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*ctx, fields.back().id);
            fields.emplace_back(std::move(field(field_name, current_id, offset)));
        }

        template<typename T>
        void emplace(const char* field_name, std::size_t offset, std::error_code& err)
        {
            constexpr type_id current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id);
                return;
            }

            fields.emplace_back(std::move(field(field_name, current_id, offset)));
        }

        void erase(std::size_t idx, std::error_code& err)
        {
            if (!field_erase(idx)) {
                err = make_error_code(errors::invalid_argument);
                return;
            }
        }

        void erase(const char* field_name, std::error_code& err)
        {
            if (!field_erase(field_name)) {
                err = make_error_code(errors::invalid_argument);
                return;
            }
        }
    };

    static inline void add_type(context& ctx, const type_declaration& type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code(errors::already_registered);
            return;
        }

        ctx.add(type.id, std::move(type_context{ type.id, type.size, type.name, type.fields }));
    }

    static inline void add_type(context& ctx, const type_declaration& type, type_id base_type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code(errors::already_registered);
            return;
        }

        ctx.add(type.id, std::move(type_context{ base_type, type.size, type.name, type.fields }));
    }

    template<typename BaseType>
    static constexpr void add_type(context& ctx, const type_declaration& type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code(errors::already_registered);
            return;
        }

        ctx.add(type.id, std::move(type_context{ obtain_id<BaseType>(), type.size, type.name, type.fields }));
    }

    template<typename T> 
    static void add_type(context& ctx, std::error_code& err)
    {
        using CleanType = grr::clean_type<T>;
        type_declaration new_type = type_declaration(ctx, grr::obtain_id<CleanType>(), grr::type_name<CleanType>());
        constexpr bool is_aggregate = std::is_aggregate<CleanType>();

#ifdef GRR_PREDECLARE_FIELDS
        if constexpr (is_aggregate) {
            constexpr bool is_visitable = visit_struct::traits::is_visitable<CleanType>::value;
            constexpr bool is_reflectable = pfr::is_implicitly_reflectable_v<CleanType, CleanType>;
            static_assert(grr::is_reflectable_v<T>, "GRR reflection supports only aggregate types (such as PODs)");

            const CleanType val = {};
            if constexpr (is_visitable) {
                visit_struct::for_each(val, [&err, &val, &new_type](const char* name, const auto& field) {
                    const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);
                    new_type.emplace<std::remove_reference_t<decltype(field)>>(name, offset, err);
                    if (err) {
                        return;
                    }

                    new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
                });
            } else if constexpr (is_reflectable) {
                pfr::for_each_field(val, [&err, &val, &new_type](const auto& field) {
                    const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);
                    
                    char field_name[16] = {};
                    std::snprintf(field_name, 16, "var%u", static_cast<std::uint32_t>(offset));

                    new_type.emplace<std::remove_reference_t<decltype(field)>>(field_name, offset, err);
                    new_type.size += sizeof(std::remove_reference_t<decltype(field)>);
                });
            }
        } else {
            if constexpr (!std::is_same_v<CleanType, void>) {
                new_type.size = sizeof(CleanType);
            }
        }
#else
        new_type.size = sizeof(CleanType);
#endif

        #ifndef GRR_RETURN_IF_FAILED
        #define GRR_RETURN_IF_FAILED(x) \
            (x); \
            if (err) { \
                return;\
            }
        #endif

        new_type.aggregate = is_aggregate;
        grr::add_type(ctx, new_type, err);
        if constexpr (!std::is_void_v<CleanType>) {
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<volatile CleanType>(), sizeof(CleanType) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<const CleanType>(), sizeof(CleanType) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<CleanType&>(), sizeof(CleanType) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<volatile CleanType&>(), sizeof(CleanType) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<const CleanType&>(), sizeof(CleanType) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<volatile CleanType const>(), sizeof(CleanType) }, err));
        }

        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<CleanType*>(), sizeof(CleanType*) }, err));
        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<volatile CleanType*>(), sizeof(CleanType*) }, err));
        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<const CleanType*>(), sizeof(CleanType*) }, err));
        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<CleanType* const>(), sizeof(CleanType*) }, err));
        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<volatile CleanType* const>(), sizeof(CleanType*) }, err));
        GRR_RETURN_IF_FAILED(grr::add_type<CleanType>(ctx, { ctx, grr::type_name<const CleanType* const>(), sizeof(CleanType*) }, err));

        #undef GRR_RETURN_IF_FAILED
    }

    namespace detail
    {
        template<typename... Types>
        static void add_types(context& ctx, std::error_code& err)
        {
            (grr::add_type<Types>(ctx, err), ...);
        }
    }

    static inline context make_context(std::error_code& err)
    {
        context out_context;
        detail::add_types<GRR_TYPES>(out_context, err);
        return out_context;
    }
}

#endif