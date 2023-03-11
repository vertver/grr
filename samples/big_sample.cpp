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
};

VISITABLE_STRUCT(another_reflected_struct, first_name, second_name);

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

int main()
{
	auto visit_fields = []<typename T>(const T& field, const char* name) {
		if constexpr (std::is_integral_v<T>) {
			std::cout << name << ": " << std::to_string(field) << std::endl;
		} else if constexpr (std::is_same_v<T, grr::string>) {
			std::cout << name << ": " << field << std::endl;
		} else if constexpr (std::is_same_v<T, grr::const_ptr_pair>) {
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
	const another_reflected_struct reflected_instance = { 1, 0 };
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
	grr::rename<my_struct>(context, "SUPER PUPER STRUCTURE");

	for (const auto& [id, type] : context) {
		const bool structured = !type.fields.empty();
		if (!type.name.compare(type.platform_name)) {
			std::cout << (structured ? "# Structure type \"" : "# Type \"") << type.name << "\" id " << id;
		} else {
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
	grr::visit(context, b, [&b_fields_count]<typename T>(const T & field, const char* name) {
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
	grr::visit(context, instance, visit_fields);

	return 0;
}