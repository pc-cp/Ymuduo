#pragma once 

/**
 * A tag class emphasises the objects are copyable.
 * The empty base class optimization applies.
 * Any derived class of copyable should be a value type.
 * 
 * 这种设计可以让开发者明确知道，这些类的对象可以通过拷贝构造函数和赋值运算符进行复制。
 * 空基类优化(EBO)可以使得派生类不因为继承了一个空基类而增加对象的大小。
 */

class copyable {
    protected:
        copyable() = default;
        ~copyable() = default;
};