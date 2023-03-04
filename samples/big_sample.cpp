#include <grr/grr.hpp>

struct my_struct
{
	int a;
};

int main()
{
	grr::context context;
	if (!grr::contains<my_struct>(context)) {

	}

	return 0;
}