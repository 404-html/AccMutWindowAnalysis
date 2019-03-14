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

#define DEFINE_VAR(name, type1, type2, init1, init2)\
template <int __name_tplarg>\
struct name {\
};\
template <>\
struct name<0> {\
    static type1 var;\
};\
type1 name<0>::var = init1;\
template <>\
struct name<1> {\
    static type2 var;\
};\
type2 name<1>::var = init2;


#define ADD_FUNC(type, name) type<__test_tplarg> name
#define ADD_VAR(type, name)\
    decltype(type<__test_tplarg>::var) name = type<__test_tplarg>::var


#endif //LLVM_TESTBASE_H
