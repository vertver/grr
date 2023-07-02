#include "grr/grr.hpp"
#include <iostream>
#include <variant>

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
	auto visit_fields = []<typename T>(const T& field, const char* name) {
		if constexpr (std::is_integral_v<T>) {
			std::cout << name << ": " << std::to_string(field) << std::endl;
		} else if constexpr (std::is_same_v<T, grr::string>) {
			std::cout << name << ": " << field << std::endl;
		} else if constexpr (grr::is_fallback_type_v<T>) {
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

	std::error_code err;
	grr::context context = grr::make_context(err);

	grr::type_declaration custom_type = grr::type_declaration(context, "My custom type");
	custom_type.emplace<int>("a", err);
	custom_type.emplace<std::uint64_t>("b", err);
	custom_type.emplace<grr::string>("string", err);
	grr::add_type(context, custom_type, err);

	grr::add_type<int_vector>(context, err);
	grr::add_type<my_struct>(context, err);
	grr::add_type<another_reflected_struct>(context, err);
	grr::add_type<b_class>(context, err);

	for (const auto& [id, type] : context) {
		const bool structured = !type.fields.empty();
		std::cout << (structured ? "# Structure type \"" : "# Type \"") << type.name << "\" id " << id;
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
	grr::reflect(context, b, err, [&b_fields_count]<typename T>(const T& field, const char* name) {
		std::cout << name << std::endl;
		b_fields_count++;
	});

	std::cout << "Detected " << std::to_string(b_fields_count) << " fields count in b_class..." << std::endl;
	std::cout << std::endl;

	grr::string* my_string = reinterpret_cast<grr::string*>(&data[custom_type.fields[2].offset]);
	my_string = new (my_string) grr::string;
	*my_string = "Test runtime string";

	//std::visit()

	auto before_stringify = grr::vector<grr::vector<int>>{ {4, 5, 234, 1}, {5, 6, 4444, 123} };
	grr::string stringified = grr::stringify(before_stringify);
	grr::vector<grr::vector<int>> unstringified = grr::unstringify<grr::vector<grr::vector<int>>>(stringified.data(), err);

	grr::reflect(context, data, custom_type.id, err, visit_fields);
	std::cout << std::endl;
	grr::reflect(context, reflected_instance, err, visit_fields);
	std::cout << std::endl;
	std::cout << "Printing before renaming...";
	std::cout << std::endl;
	grr::reflect(context, instance, err, visit_fields);
	std::cout << std::endl;
	std::cout << "Printing after renaming...";
	std::cout << std::endl;
	grr::rename<my_struct>(context, "Custom structure name", err);
	grr::rename<my_struct>(context, 0, "a", err);
	grr::rename<my_struct>(context, 1, "c", err);
	grr::rename<my_struct>(context, 2, "b", err);
	grr::rename<my_struct>(context, 3, "s1", err);
	grr::rename<my_struct>(context, 4, "s2", err);
	grr::rename<my_struct>(context, 5, "memory", err);
	grr::reflect(context, instance, err, visit_fields);
}

void run_another_test()
{
    constexpr auto type_name = grr::type_name<std::string>();
    constexpr auto stype_name = grr::type_name<another_reflected_struct>();
	constexpr auto type_hash = grr::serializable_hash<grr::type_id>(type_name);
    std::cout << type_name << std::endl;
    std::cout << type_hash << std::endl;
}

int main()
{
    //run_another_test();
	run_big_sample();
	return 0;
}