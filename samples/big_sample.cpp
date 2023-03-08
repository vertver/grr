#include <grr/grr.hpp>
#include <iostream>

struct my_struct
{
	int a;
	std::uint64_t b;
};

int main()
{
	grr::context context = grr::make_context();
	my_struct stric = {};
	stric.a = 1;
	stric.b = 2;

	grr::add_type<my_struct>(context);
	grr::visit(context, &stric, grr::obtain_id<my_struct>(), []<typename T>(const T& field, const char* name) {
		if constexpr (std::is_integral_v<T>) {
			std::cout << name << ": " << std::to_string(field) << std::endl;
		} else {
			std::cout << name << ": " << "unknown memory" << std::endl;
		}
	});

	return 0;
}