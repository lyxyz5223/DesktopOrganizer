#include "FunctionWrapper.h"

//#include <functional>
//#include <type_traits>
//
//// 类型特征：获取函数签名信息
//template <typename T>
//struct FunctionTraits;
//
//// 自由函数和函数指针的特化
//template <typename Ret, typename... Args>
//struct FunctionTraits<Ret(*)(Args...)>
//{
//    using ReturnType = Ret;
//    using ArgsTuple = std::tuple<Args...>;
//    static constexpr bool is_member = false;
//};
//
//// 函数对象的特化（operator()）
//template <typename Functor>
//struct FunctionTraits : FunctionTraits<decltype(&Functor::operator())>
//{
//};
//
//// 成员函数运算符的特化
//template <typename Class, typename Ret, typename... Args>
//struct FunctionTraits<Ret(Class::*)(Args...) const>
//{
//    using ReturnType = Ret;
//    using ArgsTuple = std::tuple<Args...>;
//    static constexpr bool is_member = true;
//};
//
//#include <any>
//#include <vector>
//#include <typeinfo>
//#include <typeindex>
//#include <memory>
//#include <iostream>
//
//class BaseFunctionContainer
//{
//public:
//    virtual ~BaseFunctionContainer() = default;
//    virtual void call() = 0;
//    virtual std::vector<std::any> getArgs() const = 0;
//    virtual bool isDeepEqual(const BaseFunctionContainer& other) const = 0;
//    virtual std::type_index getReturnType() const = 0;
//    virtual std::vector<std::type_index> getArgTypes() const = 0;
//    virtual bool sameTarget(const BaseFunctionContainer& other) const = 0;
//};
//
//template <typename Func, typename... Args>
//class FunctionContainer : public BaseFunctionContainer
//{
//    using Traits = FunctionTraits<std::decay_t<Func>>;
//
//public:
//    FunctionContainer(Func func, Args... args)
//        : func_(std::move(func)),
//        args_(std::forward<Args>(args)...) {}
//
//    void call() override
//    {
//        std::apply(func_, args_);
//    }
//
//    std::vector<std::any> getArgs() const override
//    {
//        std::vector<std::any> args;
//        std::apply([&](auto &&...arg)
//            { ((args.emplace_back(std::forward<decltype(arg)>(arg))), ...); }, args_);
//        return args;
//    }
//
//    bool isDeepEqual(const BaseFunctionContainer& other) const override
//    {
//        // 返回值类型检查
//        if (getReturnType() != other.getReturnType())
//            return false;
//
//        // 参数类型列表检查
//        const auto otherArgTypes = other.getArgTypes();
//        const auto thisArgTypes = getArgTypes();
//        if (otherArgTypes != thisArgTypes)
//            return false;
//
//        // 函数标识检查
//        if (!sameTarget(other))
//            return false;
//
//        // 参数值比较
//        return compareArgs(other);
//    }
//
//    std::type_index getReturnType() const override
//    {
//        return typeid(typename Traits::ReturnType);
//    }
//
//    std::vector<std::type_index> getArgTypes() const override
//    {
//        return { typeid(Args)... };
//    }
//
//    bool sameTarget(const BaseFunctionContainer& other) const override
//    {
//        if (auto pOther = dynamic_cast<const FunctionContainer*>(&other))
//        {
//            return compareTarget(pOther->func_, func_);
//        }
//        return false;
//    }
//
//private:
//    // 自由函数指针比较
//    template <typename Ret, typename... FArgs>
//    static bool compareTarget(Ret(*p1)(FArgs...), Ret(*p2)(FArgs...))
//    {
//        return p1 == p2;
//    }
//
//    // 函数对象比较（通过typeid）
//    template <typename F1, typename F2>
//    static bool compareTarget(const F1&, const F2&)
//    {
//        return typeid(F1) == typeid(F2);
//    }
//
//    bool compareArgs(const BaseFunctionContainer& other) const
//    {
//        try
//        {
//            const auto& otherArgs = dynamic_cast<const FunctionContainer&>(other).args_;
//            return args_ == otherArgs;
//        }
//        catch (...)
//        {
//            return false;
//        }
//    }
//
//    Func func_;
//    std::tuple<Args...> args_;
//};
//
////工厂函数
////创建容器实例的辅助函数：
//template <typename Func, typename... Args>
//std::unique_ptr<BaseFunctionContainer> makeFunctionContainer(Func&& func, Args &&...args)
//{
//    return std::make_unique<FunctionContainer<std::decay_t<Func>, std::decay_t<Args>...>>(
//        std::forward<Func>(func),
//        std::forward<Args>(args)...);
//}
//
//// 示例函数
////int add(int a, int b) { return a + b; }
////int sub(int a, int b) { return a - b; }
////
////int main()
////{
////    std::cout << std::boolalpha;
////    // 相同函数相同参数
////    auto fc1 = makeFunctionContainer(add, 2, 3);
////    auto fc2 = makeFunctionContainer(add, 2, 3);
////    std::cout << "fc1 == fc2: " << fc1->isDeepEqual(*fc2) << "\n"; // true
////
////    // 不同函数相同参数
////    auto fc3 = makeFunctionContainer(sub, 2, 3);
////    std::cout << "fc1 == fc3: " << fc1->isDeepEqual(*fc3) << "\n"; // false
////
////    // 相同函数不同参数
////    auto fc4 = makeFunctionContainer(add, 5, 4);
////    std::cout << "fc1 == fc4: " << fc1->isDeepEqual(*fc4) << "\n"; // false
////
////    // Lambda表达式比较
////    auto lambda1 = [](int x)
////        { return x * 2; };
////    auto lambda2 = [](int x)
////        { return x * 2; };
////    auto fc5 = makeFunctionContainer(lambda1, 5);
////    auto fc6 = makeFunctionContainer(lambda1, 5);
////    auto fc7 = makeFunctionContainer(lambda2, 5);
////    std::cout << "fc5 == fc6: " << fc5->isDeepEqual(*fc6) << "\n"; // true
////    std::cout << "fc5 == fc7: " << fc5->isDeepEqual(*fc7) << "\n"; // false
////    system("pause");
////    return 0;
////}
