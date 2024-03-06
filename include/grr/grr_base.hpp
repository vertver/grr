/***************************************************************************************
* Copyright (C) Anton Kovalev (vertver), 2023. All rights reserved.
* GRR - "Games Require Reflection", library for integrating reflection into games
* Boost Software License, Version 1.0
***************************************************************************************/
#ifndef GRR_BASE_HPP_INCLUDED
#define GRR_BASE_HPP_INCLUDED

namespace grr
{
    template<typename T>
    static constexpr auto type_name()
    {
        if constexpr (reflectable<T>()) {
            return nameof::nameof_short_type<T>();
        } else {
            return nameof::nameof_type<T>();
        }
    }

    enum class errors : int
    {
        invalid_argument,
        invalid_type,
        invalid_ordering,
        unregistered_id,
        already_registered,
        parsing_failed,
        out_of_range
    };

    struct error_category : public std::error_category
    {
        string_view tname;

        error_category(const string_view& err) : std::error_category()
        {
            tname = err;
        }

        std::string message(int c) const
        {
            static const char* err_msg[] =
            {
                "Invalid argument",
                "Invalid type",
                "Invalid ordering",
                "Unregisted ID",
                "Already registered",
                "Parsing failed",
                "Out of range"
            };

            return std::string(err_msg[c]) + std::string(tname.begin(), tname.end());
        }

        const char* name() const noexcept { return "GRR Error code"; }
    };

    template<typename T>
    static inline std::error_code make_error_code(errors e)
    {
        return std::error_code(static_cast<int>(e), error_category(type_name<T>()));
    }   
    
    static inline std::error_code make_error_code(errors e, const string_view& name)
    {
        return std::error_code(static_cast<int>(e), error_category(name));
    }

    struct field
    {
        GRR_CONSTEXPR field(const string_view& new_name, typeid_t new_id, std::size_t new_offset, const vector<tag_t>& new_tags)
            : name(new_name), id(new_id), offset(new_offset), tags(new_tags) {}

        GRR_CONSTEXPR field(string_view&& new_name, typeid_t&& new_id, std::size_t&& new_offset, vector<tag_t>&& new_tags)
            : name(new_name), id(new_id), offset(new_offset), tags(new_tags) {}

        field() = default;
        field(const field&) = default;
        field(field&&) = default;
        field& operator=(const field&) = default;
        field& operator=(field&&) = default;

        std::size_t offset;
        typeid_t id;
        string name;
        vector<tag_t> tags;
    };

    struct type_context
    {
        bool aggregate;
        typeid_t base_type;
        std::size_t size;
        string name;
        vector<tag_t> tags;
        vector<field> fields;
    };

    class context
    {
    private:
        using storage_map = hash_map<typeid_t, type_context>;
        hash_map<typeid_t, type_context> storage;

    public:
        const type_context& at(typeid_t id) const
        {
            return storage.at(id);
        }

        bool contains(typeid_t id) const
        {
#ifdef GRR_CXX20
            return storage.contains(id);
#else
            return storage.find(id) != storage.end();
#endif
        }

        std::size_t size(typeid_t id) const
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                return static_cast<std::size_t>(-1);
            }

            return it->second.size;
        }

        const type_context& obtain(typeid_t id) const
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

        void rename(typeid_t id, const char* new_name, std::error_code& err)
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                err = make_error_code(errors::unregistered_id, new_name);
                return;
            }

            if (it->second.aggregate) {
                err = make_error_code(errors::invalid_type, it->second.name);
                return;
            }

            it->second.name = new_name;
        }

        void rename(typeid_t id, const string_view& new_name, std::error_code& err)
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                err = make_error_code(errors::unregistered_id, new_name);
                return;
            }

            if (it->second.aggregate) {
                err = make_error_code(errors::invalid_type, it->second.name);
                return;
            }

            it->second.name = string(new_name.begin(), new_name.end());
        }

        void emplace(typeid_t id, const type_context& type)
        {
            storage.emplace(std::make_pair(id, type));
        }

        void emplace(typeid_t id, type_context&& type)
        {
            storage.emplace(std::make_pair(id, type));
        }

        void erase(typeid_t id, std::error_code& err)
        {
            const auto& it = storage.find(id);
            if (it == storage.end()) {
                err = make_error_code(errors::unregistered_id, "");
                return;
            }

            if (it->second.aggregate) {
                err = make_error_code(errors::invalid_type, it->second.name);
                return;
            }

            storage.erase(id);
        }
    };

    static inline string_view type_name(const context& ctx, typeid_t id)
    {
        if (!ctx.contains(id)) {
            return "";
        }

        return string_view(ctx.at(id).name.begin(), ctx.at(id).name.end());
    }

    template<typename T>
    static constexpr bool reflectable()
    {
        return grr::is_reflectable_v<T>;
    }

    static inline bool reflectable(const grr::context& ctx, grr::typeid_t id)
    {
        if (!ctx.contains(id)) {
            return false;
        }

        return !ctx.at(id).fields.empty();
    }

    template<typename T>
    static inline grr::string runtime_type_name()
    {
        constexpr auto name = type_name<T>();
        return grr::string(name.begin(), name.end());
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

    static constexpr typeid_t obtain_id(const grr::string_view& name)
    {
        return binhash<typeid_t>(name);
    }

    template<typename T>
    static constexpr typeid_t obtain_id()
    {
        return obtain_id(type_name<grr::clean_type<T>>());
    }

    static inline std::size_t size(const context& ctx, typeid_t id)
    {
        return ctx.size(id);
    }

    template<typename T>
    static inline bool contains(const context& ctx)
    {
        constexpr typeid_t id = obtain_id<grr::clean_type<T>>();
        return ctx.contains(id);
    }

    static inline void erase(context& ctx, typeid_t id, std::error_code& err)
    {
        ctx.erase(id, err);
    }

    static inline bool contains(const context& ctx, typeid_t id)
    {
        return ctx.contains(id);
    }

    static inline bool contains(const context& ctx, const grr::string_view& name)
    {
        return ctx.contains(binhash<typeid_t>(name));
    }

    static inline void rename(context& ctx, typeid_t id, const char* new_name, std::error_code& err)
    {
        ctx.rename(id, new_name, err);
    }

    static inline void rename(context& ctx, typeid_t id, const string_view& new_name, std::error_code& err)
    {
        ctx.rename(id, new_name, err);
    }    

    template<typename T>
    static inline void rename(context & ctx, const char* new_name, std::error_code & err)
    {
        constexpr typeid_t id = obtain_id<grr::clean_type<T>>();
        ctx.rename(id, new_name, err);
    }

    template<typename T>
    static inline void rename(context& ctx, const string_view& new_name, std::error_code& err)
    {
        constexpr typeid_t id = obtain_id<grr::clean_type<T>>();
        ctx.rename(id, new_name, err);
    }

    static inline std::size_t offset(const context& ctx, typeid_t id, std::size_t field_idx, std::error_code& err)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id, "");
            return 0;
        }

        auto& fields = ctx.at(id).fields;
        if (field_idx >= fields.size()) {
            err = make_error_code(errors::invalid_argument, ctx.at(id).name);
            return 0;
        }

        return fields.at(field_idx).offset;
    }

    static inline typeid_t base_type(context& ctx, typeid_t id)
    {
        return ctx.at(id).base_type;
    }

    template<typename T>
    static constexpr typeid_t base_type()
    {
        return obtain_id<std::remove_pointer_t<grr::clean_type<T>>>();
    }

    template<typename T>
    static inline bool size(const context& ctx)
    {
        return ctx.size(obtain_id<grr::clean_type<T>>());
    }

    template<typename T>
    static inline std::size_t offset(const context& ctx, std::size_t field_idx, std::error_code& err)
    {
        constexpr typeid_t id = obtain_id<grr::clean_type<T>>();
        return offset(ctx, id, field_idx, err);
    }

#ifdef GRR_PREDECLARE_FIELDS
    template<typename T, typename Function>
    static constexpr void visit(const grr::context& ctx, T& data, std::error_code& err, Function&& func)
    {
        using CleanType = grr::clean_type<T>;

        auto call_function = []<typename Func>(Func&& in_func, auto argument, const char* name) -> bool {
            using ArgumentLReference = std::add_lvalue_reference_t<decltype(*argument)>;
            constexpr bool callable_1 = std::is_invocable_r_v<bool, Func, ArgumentLReference>;
            constexpr bool callable_2 = std::is_invocable_r_v<bool, Func, ArgumentLReference, const char*>;
            constexpr bool callable_3 = std::is_invocable_r_v<void, Func, ArgumentLReference>;
            constexpr bool callable_4 = std::is_invocable_r_v<void, Func, ArgumentLReference, const char*>;
            static_assert(callable_1 || callable_2 || callable_3 || callable_4, "Captured function is not accepted");

            if constexpr (callable_1) {
                return in_func(*argument);
            } else if constexpr (callable_2) {
                return in_func(*argument, name);
            } else if constexpr (callable_3) {
                in_func(*argument);
                return true;
            } else if constexpr (callable_4) {
                in_func(*argument, name);
                return true;
            } else {
                return false;
            }
        };

        constexpr typeid_t id = grr::obtain_id<T>();
        if (!ctx.contains(id)) {
            err = make_error_code<T>(errors::unregistered_id);
            return;
        }

        const auto& type_info = ctx.at(id);
        if (type_info.fields.empty()) {
            err = make_error_code<T>(errors::invalid_type);
            return;
        }

        if constexpr (boost::pfr::is_implicitly_reflectable_v<CleanType, CleanType>) {
            boost::pfr::for_each_field(data,
                [&err, &type_info, &call_function, &func]<typename U>(U& field, std::size_t index) {
                if (err || index >= type_info.fields.size()) {
                    err = make_error_code<T>(errors::invalid_ordering);
                    return;
                }

                const auto& field_info = type_info.fields.at(index);
                if (grr::obtain_id<U>() != field_info.id) {
                    err = make_error_code<T>(errors::invalid_type);
                    return;
                }

                call_function(func, &field, field_info.name.c_str());
            });
        } else {
            err = make_error_code<T>(errors::invalid_type);
        }
    }

    template<typename T, typename Function>
    static inline void visit_raw(T& data, std::error_code& err, Function&& func)
    {
        using CleanType = grr::clean_type<T>;
        
        auto call_function = [](auto&& func, auto argument, const char* name) -> bool {
            using ArgumentLReference = std::add_lvalue_reference_t<decltype(*argument)>;
            constexpr bool callable_1 = std::is_invocable_r_v<bool, decltype(func), ArgumentLReference>;
            constexpr bool callable_2 = std::is_invocable_r_v<bool, decltype(func), ArgumentLReference, const char*>;
            constexpr bool callable_3 = std::is_invocable_r_v<void, decltype(func), ArgumentLReference>;
            constexpr bool callable_4 = std::is_invocable_r_v<void, decltype(func), ArgumentLReference, const char*>;
            static_assert(callable_1 || callable_2 || callable_3 || callable_4, "Captured function is not accepted");

            if constexpr (callable_1) {
                return func(*argument);
            } else if constexpr (callable_2) {
                return func(*argument, name);
            } else if constexpr (callable_3) {
                func(*argument);
                return true;
            } else if constexpr (callable_4) {
                func(*argument, name);
                return true;
            } else {
                return false;
            }
        };

        if constexpr (boost::pfr::is_implicitly_reflectable_v<CleanType, CleanType>) {
            boost::pfr::for_each_field(data, [&data, &call_function, &func, &err](auto& field, std::size_t index) {
                constexpr auto field_names = boost::pfr::names_as_array<CleanType>();
                if (call_function(func, &field, field_names.at(index).data())) {
                    err = make_error_code<T>(errors::invalid_argument);
                }
            });
        } else {
            err = make_error_code<T>(errors::invalid_type);
        }
    }
#endif

    namespace detail
    {
        template<typename T, typename Function, typename Data>
        static constexpr void visit_static(Data data, typeid_t id, const char* name, bool& called, Function&& func)
        {
            using CleanDataType = std::remove_pointer_t<decltype(data)>;
            auto call_function = [](auto&& func, auto argument, const char* name) -> bool {
                // #TODO: (auto& field, std::size_t idx)
                using ArgumentLReference = std::add_lvalue_reference_t<decltype(*argument)>;
                constexpr bool callable_1 = std::is_invocable_r_v<bool, decltype(func), ArgumentLReference>;
                constexpr bool callable_2 = std::is_invocable_r_v<bool, decltype(func), ArgumentLReference, const char*>;
                constexpr bool callable_3 = std::is_invocable_r_v<void, decltype(func), ArgumentLReference>;
                constexpr bool callable_4 = std::is_invocable_r_v<void, decltype(func), ArgumentLReference, const char*>;
                static_assert(callable_1 || callable_2 || callable_3 || callable_4, "Captured function is not accepted");

                if constexpr (callable_1) {
                    return func(*argument);
                } else if constexpr (callable_2) {
                    return func(*argument, name);
                } else if constexpr (callable_3) {
                    func(*argument);
                    return true;
                } else if constexpr (callable_4) {
                    func(*argument, name);
                    return true;
                }

                return false;
            };

            if (called) {
                return;
            }

            constexpr typeid_t current_ptr_id = grr::obtain_id<T*>();

            if constexpr (!std::is_same_v<T, void>) {
                constexpr typeid_t current_id = grr::obtain_id<T>();
                if (current_id == id) {
                    if constexpr (std::is_const_v<CleanDataType>) {
                        called = call_function(func, reinterpret_cast<const T*>(data), name);
                    } else {
                        called = call_function(func, reinterpret_cast<T*>(data), name);
                    }
                }

                if (called) {
                    return;
                }

                constexpr typeid_t vector_id = grr::obtain_id<grr::vector<grr::vector<T>>>();
                constexpr typeid_t vectored_vector_id = grr::obtain_id<grr::vector<T>>();
                if (id == vector_id) {
                    if constexpr (std::is_const_v<CleanDataType>) {
                        called = call_function(func, reinterpret_cast<const grr::vector<T>*>(reinterpret_cast<size_t>(data)), name);
                    } else {
                        called = call_function(func, reinterpret_cast<grr::vector<T>*>(reinterpret_cast<size_t>(data)), name);
                    }
                }

                if (called) {
                    return;
                }

                if (id == vectored_vector_id) {
                    if constexpr (std::is_const_v<CleanDataType>) {
                        called = call_function(func, reinterpret_cast<const grr::vector<grr::vector<T>>*>(reinterpret_cast<size_t>(data)), name);
                    }
                    else {
                        called = call_function(func, reinterpret_cast<grr::vector<grr::vector<T>>*>(reinterpret_cast<size_t>(data)), name);
                    }
                }
            }

            if (called) {
                return;
            }

            if (current_ptr_id == id) {
                if constexpr (std::is_const_v<CleanDataType>) {
                    called = call_function(func, reinterpret_cast<const T**>(reinterpret_cast<size_t>(data)), name);
                } else {
                    called = call_function(func, reinterpret_cast<T**>(reinterpret_cast<size_t>(data)), name);
                }
            }

            if (called) {
                return;
            }

            constexpr typeid_t vector_ptr_id = grr::obtain_id<grr::vector<grr::vector<T*>>>();
            constexpr typeid_t vectored_vector_ptr_id = grr::obtain_id<grr::vector<T*>>();
            if (id == vector_ptr_id) {
                if constexpr (std::is_const_v<CleanDataType>) {
                    called = call_function(func, reinterpret_cast<const grr::vector<T*>*>(reinterpret_cast<size_t>(data)), name);
                }
                else {
                    called = call_function(func, reinterpret_cast<grr::vector<T*>*>(reinterpret_cast<size_t>(data)), name);
                }
            }

            if (called) {
                return;
            }

            if (id == vectored_vector_ptr_id) {
                if constexpr (std::is_const_v<CleanDataType>) {
                    called = call_function(func, reinterpret_cast<const grr::vector<grr::vector<T*>>*>(reinterpret_cast<size_t>(data)), name);
                }
                else {
                    called = call_function(func, reinterpret_cast<grr::vector<grr::vector<T*>>*>(reinterpret_cast<size_t>(data)), name);
                }
            }
        }

        template<typename... Types, typename Function, typename Data>
        static constexpr bool visit_static(Data data, const char* name, typeid_t id, Function&& func)
        {
            bool called = false;
            (visit_static<Types>(data, id, name, called, func), ...);
            return called;
        }

        template<typename... Types, typename Function, typename Data>
        static constexpr bool visit_static(Data data, typeid_t id, Function&& func)
        {
            bool called = false;
            (visit_static<Types>(data, id, "var", called, func), ...);
            return called;
        }

#ifdef GRR_PREDECLARE_FIELDS
        template<typename T, typename Function, typename Data>
        static constexpr void visit_static_reflectable(const grr::context& ctx, typeid_t id, Data data, Function&& func, bool& called)
        {
            using CleanType = grr::clean_type<T>;
            if constexpr (grr::is_reflectable_v<CleanType>) {
                if (called) {
                    return;
                }

                visit_static<CleanType>(&data, id, "dummy", called, [&ctx, &func](auto& value) {
                    if constexpr (std::is_same_v<grr::clean_type<decltype(value)>, CleanType>) {
                        std::error_code err;
                        grr::visit<CleanType>(ctx, value, err, func);
                    }
                });
            }
        }

        template<typename... Types, typename Function, typename Data>
        static constexpr bool visit_static_reflectable(const grr::context& ctx, typeid_t id, Data data, Function&& func)
        {
            bool called = false;
            (visit_static_reflectable<Types>(ctx, id, data, func, called), ...);
            return called;
        }
#endif

        template<std::size_t recursion_level = 0, typename Function, typename Data>
        static inline void visit(const grr::context& ctx, Data data, typeid_t id, std::error_code& err, Function&& func)
        {
            // [](auto& field, const char* name)
            auto call_function = [](auto&& func, auto ptr, auto id, auto size) -> bool {
                auto pair = std::make_pair(size, std::make_pair(ptr, id));
                
                // #TODO: (auto& field, std::size_t idx)
                using PairLReference = std::add_lvalue_reference_t<decltype(pair)>;
                constexpr bool callable_1 = std::is_invocable_r_v<bool, decltype(func), PairLReference>;
                constexpr bool callable_2 = std::is_invocable_r_v<bool, decltype(func), PairLReference, const char*>;
                constexpr bool callable_3 = std::is_invocable_r_v<void, decltype(func), PairLReference>;
                constexpr bool callable_4 = std::is_invocable_r_v<void, decltype(func), PairLReference, const char*>;
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
                    err = make_error_code<Data>(errors::unregistered_id);
                    return;
                }
    
                if (!call_function(func, type_ptr, id, type_info.size)) {
                    err = make_error_code<Data>(errors::invalid_argument);
                    return;
                }
            } else {
#ifdef GRR_PREDECLARE_FIELDS
                if (visit_static_reflectable<GRR_TYPES>(ctx, id, data, func)) {
                    return;
                }
#endif

                for (const auto& cfield : type_info.fields) {
                    auto field_ptr = data;
                    const typeid_t field_id = cfield.id;
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
                            err = make_error_code<Data>(errors::unregistered_id);
                            return;
                        }

                        auto& field_type = ctx.obtain(field_id);
                        if (!call_function(func, field_ptr, field_id, field_type.size)) {
                            err = make_error_code<Data>(errors::invalid_argument);
                            return;
                        }
                    }
                }
            }
        }
    }

    template<std::size_t recursion_level = 0, typename Function, typename Data>
    static inline void visit(const grr::context& ctx, Data data, typeid_t id, std::error_code& err, Function&& func)
    {
        if (!ctx.contains(id)) {
            err = make_error_code<Data>(errors::unregistered_id);
            return;
        }

        grr::detail::visit<recursion_level>(ctx, data, id, err, func);
        if (err) {
            return;
        }
    }

    template<typename T, typename... Args>
    static inline void construct(T* memory_to_construct, Args... args)
    {
        new (memory_to_construct) T(args...);
    }

    template<typename T, typename... Args>
    static inline void destruct(T* memory_to_construct)
    {
        memory_to_construct->~T();
    }

    static inline void construct(const grr::context& ctx, void* memory_to_construct, typeid_t id, std::error_code& err)
    {
        grr::visit(ctx, memory_to_construct, id, err, [](auto& field) {
            using CleanType = grr::clean_type<decltype(field)>;
            if constexpr (!grr::is_fallback_type_v<CleanType>) {
                grr::construct<CleanType>(&field);
            }
        });
    }

    static inline void destruct(const grr::context& ctx, void* memory_to_destruct, typeid_t id, std::error_code& err)
    {
        grr::visit(ctx, memory_to_destruct, id, err, [](auto& field) {
            using CleanType = grr::clean_type<decltype(field)>;
            if constexpr (!grr::is_fallback_type_v<CleanType>) {
                grr::destruct<CleanType>(&field);
            }
        });
    }

    class type_declaration
    {
    public:
        bool aggregate = false;
        const context* ctx;
        std::int64_t index;
        std::size_t size;
        string name;
        typeid_t id;
        vector<field> fields;
        vector<tag_t> tags;

    private:
        bool field_erase(const string_view& field_name)
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

    public:
        GRR_CONSTEXPR type_declaration() = delete;
        GRR_CONSTEXPR type_declaration(type_declaration&) = delete;
        GRR_CONSTEXPR type_declaration(const type_declaration&) = delete;
        GRR_CONSTEXPR type_declaration(type_declaration&&) = default;

        GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(0), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, const string_view& type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(obtain_id(type_name)), size(new_size), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, typeid_t in_id, const string_view& type_name) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(in_id), size(0), index(-1) {}

        GRR_CONSTEXPR type_declaration(const context& in_context, typeid_t in_id, const string_view& type_name, std::size_t new_size) noexcept
            : ctx(&in_context), name(type_name.begin(), type_name.end()), id(in_id), size(new_size), index(-1) {}

        void emplace(const string_view& field_name, grr::typeid_t id, std::error_code& err)
        {
            if (!grr::contains(*ctx, id)) {
                err = make_error_code(errors::unregistered_id, name);
                return;
            }

            const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*ctx, fields.back().id);
            fields.emplace_back(std::move(field(field_name, id, offset, {})));
        }

        template<typename T>
        void emplace(const string_view& field_name, std::error_code& err)
        {
            constexpr typeid_t current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id, name);
                return;
            }

            const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*ctx, fields.back().id);
            fields.emplace_back(std::move(field(field_name, current_id, offset, {})));
        }     
        
        template<typename T>
        void emplace(const string_view& field_name, const vector<tag_t>& tags, std::error_code& err)
        {
            constexpr typeid_t current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id, name);
                return;
            }

            const std::size_t offset = fields.empty() ? 0 : fields.back().offset + grr::size(*ctx, fields.back().id);
            fields.emplace_back(std::move(field(field_name, current_id, offset, tags)));
        }

        template<typename T>
        void emplace(const string_view& field_name, std::size_t offset, std::error_code& err)
        {
            constexpr typeid_t current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id, name);
                return;
            }

            fields.emplace_back(std::move(field(field_name, current_id, offset, {})));
        }
                
        template<typename T>
        void emplace(const string_view& field_name, std::size_t offset, const vector<tag_t>& tags, std::error_code& err)
        {
            constexpr typeid_t current_id = obtain_id<T>();
            if (!grr::contains(*ctx, current_id)) {
                err = make_error_code(errors::unregistered_id, name);
                return;
            }

            fields.emplace_back(std::move(field(field_name, current_id, offset, tags)));
        }

        void erase(std::size_t idx, std::error_code& err)
        {
            if (!field_erase(idx)) {
                err = make_error_code(errors::invalid_argument, name);
                return;
            }
        }

        void erase(const string_view& field_name, std::error_code& err)
        {
            if (!field_erase(field_name)) {
                err = make_error_code(errors::invalid_argument, name);
                return;
            }
        }
    };

    static inline void remove_type(context& ctx, typeid_t id, std::error_code& err)
    {
        if (!ctx.contains(id)) {
            err = make_error_code(errors::unregistered_id, "");
            return;
        }

        ctx.erase(id, err);
    }  
    
    template<typename T>
    static inline void remove_type(context& ctx, std::error_code& err)
    {
        constexpr typeid_t current_id = obtain_id<T>();
        if (!ctx.contains(current_id)) {
            err = make_error_code<T>(errors::unregistered_id);
            return;
        }

        ctx.erase(current_id, err);
    }

    static inline void add_type(context& ctx, const type_declaration& type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code(errors::already_registered, "");
            return;
        }

        ctx.emplace(type.id, std::move(type_context{ type.aggregate, type.id, type.size, type.name, type.tags, type.fields }));
    }

    static inline void add_type(context& ctx, const type_declaration& type, typeid_t base_type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code(errors::already_registered, "");
            return;
        }

        ctx.emplace(type.id, std::move(type_context{ type.aggregate, base_type, type.size, type.name, type.tags, type.fields }));
    }

    template<typename BaseType>
    static constexpr void add_type(context& ctx, const type_declaration& type, std::error_code& err)
    {
        if (ctx.contains(type.id)) {
            err = make_error_code<BaseType>(errors::already_registered);
            return;
        }

        ctx.emplace(type.id, std::move(type_context{ type.aggregate, obtain_id<BaseType>(), type.size, type.name, type.tags, type.fields }));
    }

    template<typename T> 
    static void add_type(context& ctx, std::error_code& err)
    {
        using CleanType = grr::clean_type<T>;
        type_declaration new_type = type_declaration(ctx, grr::obtain_id<CleanType>(), grr::type_name<CleanType>());
        constexpr bool is_aggregate = std::is_aggregate<CleanType>();

#ifdef GRR_PREDECLARE_FIELDS
        if constexpr (is_aggregate) {
            static_assert(grr::is_reflectable_v<CleanType>, "GRR compile-time reflection supports only aggregate types (such as PODs).");

            const CleanType val = {};
            if constexpr (boost::pfr::is_implicitly_reflectable_v<CleanType, CleanType>) {
                boost::pfr::for_each_field(val, [&err, &val, &new_type]<typename U>(const U& field, std::size_t index) {
                    constexpr auto field_names = boost::pfr::names_as_array<CleanType>();
                    const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(&field) - reinterpret_cast<std::ptrdiff_t>(&val);

                    new_type.emplace<grr::clean_type<U>>(field_names.at(index), offset, err);
                    if (err) {
                        return;
                    }

                    new_type.size += sizeof(grr::clean_type<U>);
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
        GRR_RETURN_IF_FAILED(grr::add_type(ctx, new_type, err));
        if constexpr (!std::is_same_v<CleanType, void>) {
            GRR_RETURN_IF_FAILED(grr::add_type<grr::vector<CleanType>>(ctx, { ctx, grr::type_name<grr::vector<CleanType>>(), sizeof(grr::vector<CleanType>) }, err));
            GRR_RETURN_IF_FAILED(grr::add_type< grr::vector<grr::vector<CleanType>>>(ctx, { ctx, grr::type_name<grr::vector<grr::vector<CleanType>>>(), sizeof(grr::vector<grr::vector<CleanType>>) }, err));
        }

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

    static inline std::unique_ptr<context> make_context_ptr(std::error_code& err)
    {
        std::unique_ptr<context> out_context = std::make_unique<context>();
        detail::add_types<GRR_TYPES>(*out_context, err);
        return out_context;
    }
}

#endif