// #include <iostream>
// #include <memory>

// class Foo : public std::enable_shared_from_this<Foo> {
//     public:
//         std::shared_ptr<Foo> getPtr() {
//             return shared_from_this();   
//             // return std::shared_ptr<Foo>(this); // free(): double free detected in tcache 2
//         }
//         ~Foo() {
//             std::cout << "Good::~Good() called" << std::endl;
//         }
// };

// int main() {
//     {
//         std::shared_ptr<Foo> sp1(new Foo());
//         std::shared_ptr<Foo> sp2 = sp1->getPtr();

//         std::cout << sp1.use_count() << std::endl;
//         std::cout << sp2.use_count() << std::endl;
//     }
// }