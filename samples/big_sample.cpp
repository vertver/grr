#include <grr/grr.hpp>

struct my_struct
{
	int a;
};

int main()
{
	grr::context context;


	if (!grr::contains<my_struct>(context)) {
		grr::type new_type = grr::type(context);
		new_type.name(grr::type_name<my_struct>());
		new_type.emplace<int>("a");
	}

	return 0;
}