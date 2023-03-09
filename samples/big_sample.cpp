#include <grr/grr.hpp>
#include <iostream>

struct my_struct
{
	int a;
	std::uint64_t b;
	grr::string s1;
	grr::string s2;
};

int main()
{
	const my_struct instance = { 1, 2, "hello reflection", "under reflection" };

	grr::context context = grr::make_context();
	grr::add_type<my_struct>(context);
	grr::rename<my_struct>(context, "SUPER PUPER STRUCTURE");

	for (const auto& [id, type] : context) {
		if (!type.display_name.compare(type.real_name)) {
			std::cout << "Type \"" << type.display_name << "\" id " << id << std::endl;
		} else {
			std::cout << "Type \"" << type.display_name << "\" (" << type.real_name << ") id " << id << std::endl;
		}
	}
	std::cout << std::endl;

	grr::visit(context, instance, []<typename T>(const T& field, const char* name) {
		if constexpr (std::is_integral_v<T>) {
			std::cout << name << ": " << std::to_string(field) << std::endl;
		} else if constexpr (std::is_same_v<T, grr::string>) {
			std::cout << name << ": " << field << std::endl;
		} else {
			std::cout << name << ": " << "unknown memory" << std::endl;
		}
	});

	return 0;
}