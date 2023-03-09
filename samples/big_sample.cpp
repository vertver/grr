#include <grr/grr.hpp>
#include <iostream>

struct my_struct
{
	int a;
	std::uint64_t b;
	grr::string s;
};

int main()
{
	grr::context context = grr::make_context();
	my_struct instance = {};
	instance.a = 1;
	instance.b = 2;
	instance.s = "hello reflection";

	grr::add_type<my_struct>(context);
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