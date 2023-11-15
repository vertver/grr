# GRR - The reflection for games
![image](grr_logo.png)

# Current status of GRR
GRR is a **very** experimental library. You can use it in your project, but it is at your own risk (I can't guarantee that there are no bugs in this library). However, if you've found a bug and want to contribute, you are welcome to do so. Create an issue on GitHub or PR if you have already developed a fix or feature.

---

# How does it work?
GRR uses multiple techniques to obtain information about structure on compilation time. 
The first reflection method was described by [Anton Polukhin](https://github.com/apolukhin) at [Meeting C++ 2018](https://www.youtube.com/watch?v=UlNUNxLtBI0). This reflection method works through C++14 hack, which allows to represent a structure as `std::tuple`, which allows to access fields of the structure through indexes. The limitation of this method is the lack of field name information and the ability to work only with C-like structure (or otherwise PODs).

The second method was implemented by [cbeck88](https://github.com/cbeck88) in [visit_struct](https://github.com/cbeck88/visit_struct) library, and it works from C++11 and higher. This method of reflection has the same advantages as the first one, with the only difference that we have access to the names of the structure and its fields. The disadvantage of this method is that it is necessary to create the structure through special macros or use a special `VISITABLE_STRUCT` macro to define all the fields of the structure.

The goal of GRR is to combine these methods and use their advantages. In addition, GRR has its own analogue of RTTI, which is compatible with compile-time types and is portable between compilers because it uses a limited representation of the type name through the `nameof` library. 

---

# Examples
First of all, very basic example of how to use this library:
```cpp
#include <grr/grr.hpp>

struct TestStruct
{
    int test_1;
    float test_2;
    std::string test_3;
};

int main()
{
    std::error_code err;
    auto context = grr::make_context(err);
    if (err) {
        std::cout << err.message();
        return -1;
    }

    grr::add_type<TestStruct>(context);

    TestStruct test = {};
    test.test_1 = 1;   
    test.test_2 = 2.0f;
    test.test_3 = "test";
    grr::visit(context, test, err, [](const auto& field, const char* name){
        using CleanType = grr::clean_type<decltype(field)>;
        if constexpr (std::is_same_v<CleanType, std::string>) {
            std::cout << name << ": " << field;
        } else if constexpr (std::is_integral_v<CleanType>) {
            std::cout << name << ": " << std::to_string(field);
        }
    });

    return 0;
}
```

Other samples will be available in the future at [samples](https://github.com/vertver/grr/tree/master/samples) directory.

# Requirements and Limitations
GRR requires C++20 support and GRR can only be used only with C-like structures (or PODs) and can't reflect methods of structure. 

# Acknowledgments
This project uses several C++ libraries as dependencies:
[PFR](https://github.com/apolukhin/pfr_non_boost)
[visit_struct](https://github.com/cbeck88/visit_struct)
[nameof](https://github.com/Neargye/nameof)

# License
GRR is licensed under  Boost Software License, Version 1.0. Check the [LICENSE](https://github.com/vertver/grr/blob/master/LICENSE) file for more information.
