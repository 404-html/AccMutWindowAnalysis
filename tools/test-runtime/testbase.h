//
// Created by Sirui Lu on 2019-03-14.
//

#ifndef LLVM_TESTBASE_H
#define LLVM_TESTBASE_H
#define DEFINE_FUNC(name, func1, func2)\
template <int __name_tplarg>\
struct name {\
};\
template <>\
struct name<0> {\
    template <typename ...T>\
    auto operator()(T&& ...args)\
    -> decltype(func1(std::forward<T>(args)...)) {\
        return func1(std::forward<T>(args)...);\
    }\
};\
template <>\
struct name<1> {\
    template <typename ...T>\
    auto operator()(T&& ...args)\
    -> decltype(func2(std::forward<T>(args)...)) {\
        return func2(std::forward<T>(args)...);\
    }\
};

#endif //LLVM_TESTBASE_H
