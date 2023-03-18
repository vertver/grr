#include "grr/grr.hpp"
#include <iostream>

using int_vector = std::vector<int>;

struct my_struct
{
	int a;
	int c; 
	std::uint64_t b;
	grr::string s1;
	grr::string s2;
	int_vector memory;
};

struct another_reflected_struct
{
	int first_name;
	int second_name;
	void* first_ptr;
	volatile void* second_ptr;
};

GRR_REFLECT(another_reflected_struct, first_name, second_name, first_ptr, second_ptr);

class my_class
{
private:
	int i = 0;

public:
	int b = 0;

	my_class(int new_i, int new_b) : i(new_i), b(new_b) {}
};

class a_class
{
	virtual bool a() { return false; }
};

class b_class : public a_class
{
private:
	bool dds = false;

public:
	b_class(bool new_dds) : dds(new_dds) {}

	bool a() override { return dds; }
};

void run_big_sample()
{
	auto visit_fields = []<typename T>(const T & field, const char* name) {
		if constexpr (std::is_integral_v<T>) {
			std::cout << name << ": " << std::to_string(field) << std::endl;
		}
		else if constexpr (std::is_same_v<T, grr::string>) {
			std::cout << name << ": " << field << std::endl;
		}
		else if constexpr (grr::is_fallback_type_v<T>) {
			constexpr grr::type_id vector_typeid = grr::obtain_id<int_vector>();
			if (vector_typeid == field.second.second) {
				const int_vector* my_vector = reinterpret_cast<const int_vector*>(field.second.first);
				std::cout << name << ": " << "vector ( ";
				for (const auto& elem : *my_vector) {
					std::cout << std::to_string(elem) << " ";
				}
				std::cout << ")" << std::endl;
			}
		}
	};

	char data[64] = {};
	const my_struct instance = { 1, 0, 2, "hello reflection", "under reflection", { 1, 2, 3, 4 } };
	const another_reflected_struct reflected_instance = { 1, 0, nullptr, (void*)(size_t)-1 };
	const my_class class_instance = my_class(1, 0);
	const b_class b = b_class(false);

	grr::context context = grr::make_context();

	grr::type_declaration custom_type = grr::type_declaration(context, "My custom type");
	custom_type.emplace<int>("a");
	custom_type.emplace<std::uint64_t>("b");
	custom_type.emplace<grr::string>("string");
	grr::add_type(context, custom_type);

	grr::add_type<int_vector>(context);
	grr::add_type<my_struct>(context);
	grr::add_type<another_reflected_struct>(context);
	grr::add_type<b_class>(context);

	for (const auto& [id, type] : context) {
		const bool structured = !type.fields.empty();
		if (!type.name.compare(type.platform_name)) {
			std::cout << (structured ? "# Structure type \"" : "# Type \"") << type.name << "\" id " << id;
		}
		else {
			std::cout << (structured ? "# Structure type \"" : "# Type \"") << type.name << "\" (" << type.platform_name << ") id " << id;
		}

		std::cout << std::flush;
		std::cout << std::endl;
		if (structured) {
			for (const auto& field : type.fields) {
				std::cout << "    " << grr::type_name(context, field.id) << " " << field.name << ": " << std::to_string(field.offset) << std::endl;
			}
		}
	}

	std::cout << std::endl;
	std::uint64_t b_fields_count = 0;
	grr::visit(context, b, [&b_fields_count]<typename T>(const T& field, const char* name) {
		std::cout << name << std::endl;
		b_fields_count++;
	});

	std::cout << "Detected " << std::to_string(b_fields_count) << " fields count in b_class..." << std::endl;
	std::cout << std::endl;

	grr::string* my_string = reinterpret_cast<grr::string*>(&data[custom_type.fields[2].offset]);
	my_string = new (my_string) grr::string;
	*my_string = "Test runtime string";

	grr::visit(context, data, custom_type.id, visit_fields);
	std::cout << std::endl;
	grr::visit(context, reflected_instance, visit_fields);
	std::cout << std::endl;
	std::cout << "Printing befory renaming...";
	std::cout << std::endl;
	grr::visit(context, instance, visit_fields);
	std::cout << std::endl;
	std::cout << "Printing after renaming...";
	std::cout << std::endl;
	grr::rename<my_struct>(context, "SUPER PUPER STRUCTURE");
	grr::rename<my_struct>(context, 0, "a");
	grr::rename<my_struct>(context, 1, "c");
	grr::rename<my_struct>(context, 2, "b");
	grr::rename<my_struct>(context, 3, "s1");
	grr::rename<my_struct>(context, 4, "s2");
	grr::rename<my_struct>(context, 5, "memory");
	grr::visit(context, instance, visit_fields);
}

struct test_struct
{
    int a;
};

template<typename T>
struct another_test_struct
{
    T a;
};

template <std::size_t...Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
{
    return std::array{ str[Idxs]..., '\0' };
}

template<typename T>
constexpr auto compiler_type_name_array(int unused /* hack for newer versions of MSVC */)
{
    (void)(unused);

#if defined(__clang__)
    constexpr std::string_view prefix = "[T = ";
    constexpr std::string_view suffix = "]";
    constexpr std::string_view function = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    constexpr std::string_view prefix = "with T = ";
    constexpr std::string_view suffix = "]";
    constexpr std::string_view function = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view prefix = __FUNCTION__"<";
    constexpr std::string_view suffix = ">(int)";
    constexpr std::string_view function = __FUNCSIG__;
#endif
    constexpr std::size_t start = function.find(prefix) + prefix.size();
    constexpr std::size_t end = function.rfind(suffix);

    static_assert(start < end);
    constexpr std::string_view name = function.substr(start, (end - start));
    constexpr std::size_t bracket = name.find_first_of(';');
    if constexpr (bracket != std::size_t(-1)) {
        constexpr std::string_view extracted_name = function.substr(start, bracket);
        return substring_as_array(extracted_name, std::make_index_sequence<extracted_name.size()>{});
    }

    return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
    static inline constexpr auto value = compiler_type_name_array<T>(0);
};

template <typename T>
constexpr std::string_view type_name()
{
    constexpr auto& value = type_name_holder<T>::value;
    return std::string_view{ value.data(), value.size() };
}

template<typename T, std::size_t max_indexes = 64>
constexpr auto friendly_name()
{
    constexpr auto str = type_name<T>();
    constexpr std::string_view strings[] =
    {
        "struct",       // MSVC stuff
        "class",        // MSVC stuff
        "__cxx11::",    // GCC stuff
        "__cxx14::",    // GCC stuff
        "__cxx17::",    // GCC stuff
        "__cxx20::",    // GCC stuff
        "__cxx23::",    // GCC stuff
        " "             // MSVC and GCC stuff
    };

    std::string out_string;
    std::size_t indexes_count = 0;
    std::array<std::size_t, max_indexes> indexes;
    for (const std::string_view& comparable : strings) {
        std::size_t start = -1;
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
        if (current_idx < indexes_count) {
            if (i >= indexes[current_idx + 1]) {
                current_idx += 2;
            }

            if (i >= indexes[current_idx] && i < indexes[current_idx + 1] ) {
                continue;
            }
        }

        out_string.push_back(str[i]);
    }

    return out_string;
}

/*
std::string filtered_time_name()
{
    using element = std::pair<std::size_t, std::array<std::size_t, 16 + 1>>;

    auto str = type_name<std::string>();
    auto find_str = [](std::string_view str, std::string_view cmp) -> element {
        element out_elements;

        std::size_t start = -1;
        for (std::size_t i = 0; i < str.size(); i++) {
            if (out_elements.first == 16) {
                break;
            }

            if (start == -1) {
                if (str[i] == cmp[0]) {
                    if (cmp.size() == 1) {
                        out_elements.second[out_elements.first++] = i;
                        start = std::size_t(-1);
                        break;
                    }

                    start = i;
                    continue;
                }
            }
            else {
                if (i - start >= cmp.size()) {
                    out_elements.second[out_elements.first++] = start;
                    start = std::size_t(-1);
                    break;
                }

                const char& left = str[i];
                const char& right = cmp[i - start];
                if (left != right) {
                    start = std::size_t(-1);
                }
            }
        }

        return out_elements;
    };

    std::array<element, 6> elements = {
        find_str(str, "class "),
        find_str(str, "struct "),
        find_str(str, "__cxx11::"),
        find_str(str, "__cxx14::"),
        find_str(str, "__cxx17::"),
        find_str(str, "__cxx20::")
    };

    std::array<std::size_t, 6> elements_size = {
        6,
        7,
        7,
        7,
        7,
        7
    };

    std::string out_string;
    for (std::size_t i = 0; i < str.size(); i++) {
        bool break_now = false;
        for (std::size_t elem_idx = 0; elem_idx < elements.size(); elem_idx++) {
            std::size_t elem_size = elements_size[elem_idx];
            const auto& elem = elements[elem_idx];
            for (std::size_t instance_idx = 0; instance_idx < elem.first; instance_idx++) {
                std::size_t instance = elem.second[instance_idx];
                if (i >= instance && i < instance + elem_size) {
                    break_now = true;
                    break;
                }
            }

            if (break_now == true) {
                break;
            }
        }

        if (break_now) {
            continue;
        }

        //if (str[i] == ' ') {
        //    continue;
        //}

        //hash *= 0x21;
        //hash += str[i];
        out_string.push_back(str[i]);
    }

    return out_string;
}
*/

void run_another_test()
{
    //const std::string_view concated_string = concat_range<std::string>(std::index_sequence<10>{}, std::index_sequence<14>{});  
    //std::cout << "a" << std::endl;
    //std::cout << concated_string << std::endl;

    //constexpr std::size_t typei = type_id<std::size_t>();
    //std::cout << typei << std::endl;
    constexpr std::string_view full_type_name = type_name<std::string>();
    auto friendly_type_name = friendly_name<std::string>();
    std::cout << full_type_name << std::endl;
   // std::cout << type_name<test_struct>() << std::endl;
    std::cout << friendly_type_name << std::endl;
    //std::cout << filtered_time_name() << std::endl;
    //std::cout << type_name<another_test_struct<std::string>>() << std::endl;
}


int main()
{
    run_another_test();
	//run_big_sample();
	return 0;
}