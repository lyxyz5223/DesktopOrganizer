
#ifndef FUNCTIONWRAPPER_H
#define FUNCTIONWRAPPER_H

#include <any>
#include <vector>
#include <tuple>
#include <functional>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <iostream>
#include <typeindex>

// 类型特征提取
template <typename T>
struct FunctionTraits;

// 自由函数特化
template <typename Ret, typename... Args>
struct FunctionTraits<Ret(*)(Args...)> {
    using ReturnType = Ret;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr bool is_member = false;
    static constexpr bool is_function = true;
};

// 函数对象特化
template <typename Functor>
struct FunctionTraits : FunctionTraits<decltype(&Functor::operator())> {};

// 成员函数运算符特化
template <typename Class, typename Ret, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...) const> {
    using ReturnType = Ret;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr bool is_member = true;
    static constexpr bool is_function = false;
};

class FunctionWrapper {
    struct Concept {
        virtual ~Concept() = default;
        virtual void call() = 0;
        virtual std::vector<std::any> get_args() const = 0;
        virtual bool is_equal(const Concept& other) const = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual std::type_index return_type() const = 0;
        virtual std::vector<std::type_index> arg_types() const = 0;
        virtual const std::type_info& func_type() const = 0;
    };

    template <typename Func, typename... Args>
    struct Model : Concept {
        Model(Func func, Args... args)
            : func_(std::move(func)),
            args_(std::make_tuple(std::forward<Args>(args)...)) {}

        void call() override {
            std::apply(func_, args_);
        }

        std::vector<std::any> get_args() const override {
            std::vector<std::any> result;
            std::apply([&](auto&&... args) {
                (result.emplace_back(args), ...);
                }, args_);
            return result;
        }

        bool is_equal(const Concept& other) const override {
            if (auto p = dynamic_cast<const Model*>(&other)) {
                return compare_func(p->func_) && compare_args(p->args_);
            }
            return false;
        }

        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(*this);
        }

        std::type_index return_type() const override {
            using Traits = FunctionTraits<std::decay_t<Func>>;
            return typeid(typename Traits::ReturnType);
        }

        std::vector<std::type_index> arg_types() const override {
            return { typeid(Args)... };
        }

        const std::type_info& func_type() const override {
            return typeid(Func);
        }

    private:
        template <typename F>
        bool compare_func(const F& other) const {
            // 优先比较函数内容，其次比较类型
            if constexpr (std::is_pointer_v<Func>) {
                return func_ == other;  // 比较函数指针地址
            }
            else if constexpr (is_comparable<Func>::value) {
                return func_ == other;  // 使用用户定义的operator==
            }
            else {
                return typeid(func_) == typeid(other);  // 回退到类型比较
            }
        }

        bool compare_args(const std::tuple<Args...>& other) const {
            return args_ == other;
        }

        template <typename T, typename = void>
        struct is_comparable : std::false_type {};

        template <typename T>
        struct is_comparable<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>>
            : std::true_type {};

        Func func_;
        std::tuple<Args...> args_;
    };

    std::unique_ptr<Concept> ptr_;

public:
    FunctionWrapper() = default;

    template <typename Func, typename... Args,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<Func>, FunctionWrapper>>>
    FunctionWrapper(Func&& func, Args&&... args)
        : ptr_(std::make_unique<Model<std::decay_t<Func>, std::decay_t<Args>...>>(
            std::forward<Func>(func),
            std::forward<Args>(args)...)) {}

    // 拷贝支持
    FunctionWrapper(const FunctionWrapper& other)
        : ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}

    FunctionWrapper& operator=(const FunctionWrapper& other) {
        if (this != &other) {
            ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;
        }
        return *this;
    }

    // 移动支持
    FunctionWrapper(FunctionWrapper&&) noexcept = default;
    FunctionWrapper& operator=(FunctionWrapper&&) noexcept = default;

    // 执行函数
    void operator()() {
        if (ptr_) ptr_->call();
    }

    // 获取参数
    std::vector<std::any> args() const {
        return ptr_ ? ptr_->get_args() : std::vector<std::any>{};
    }

    // 严格相等比较
    bool operator==(const FunctionWrapper& other) const {
        if (!ptr_ || !other.ptr_) return ptr_ == other.ptr_;
        return ptr_->func_type() == other.ptr_->func_type() &&
            ptr_->return_type() == other.ptr_->return_type() &&
            ptr_->arg_types() == other.ptr_->arg_types() &&
            ptr_->is_equal(*other.ptr_);
    }

    bool operator!=(const FunctionWrapper& other) const {
        return !(*this == other);
    }

    // 类型信息访问
    std::type_index return_type() const {
        return ptr_ ? ptr_->return_type() : typeid(void);
    }

    std::vector<std::type_index> arg_types() const {
        return ptr_ ? ptr_->arg_types() : std::vector<std::type_index>{};
    }

    bool isNull() const {
        return ptr_.get();
    }
    //重载bool操作符
    operator bool() const {
        return isNull();
    }
};

#endif // !FUNCTIONWRAPPER_H

// 测试用例
//struct Calculator {
//    int base;
//    int operator()(int x) const { return x + base; }
//    bool operator==(const Calculator& other) const { return base == other.base; }
//};
//
//int add(int a, int b) { return a + b; }
//int sub(int a, int b) { return a - b; }
//
//int main() {
//    // 基本函数测试
//    FunctionWrapper f1(add, 2, 3);
//    FunctionWrapper f2(add, 2, 3);
//    FunctionWrapper f3(sub, 2, 3);
//    std::cout << std::boolalpha;
//    std::cout << "f1 == f2: " << (f1 == f2) << "\n"; // true
//    std::cout << "f1 == f3: " << (f1 == f3) << "\n"; // false
//
//    // 状态函数对象测试
//    Calculator c1{ 5 }, c2{ 5 }, c3{ 10 };
//    FunctionWrapper f4(c1, 10);
//    FunctionWrapper f5(c2, 10);
//    FunctionWrapper f6(c3, 10);
//    std::cout << "f4 == f5: " << (f4 == f5) << "\n"; // true
//    std::cout << "f4 == f6: " << (f4 == f6) << "\n"; // false
//
//    // Lambda测试
//    auto lambda1 = [](int x) { return x * 2; };
//    auto lambda2 = [](int x) { return x * 2; };
//    FunctionWrapper f7(lambda1, 5);
//    FunctionWrapper f8(lambda1, 5);
//    FunctionWrapper f9(lambda2, 5);
//    std::cout << "f7 == f8: " << (f7 == f8) << "\n"; // true
//    std::cout << "f7 == f9: " << (f7 == f9) << "\n"; // false
//
//    // 参数访问测试
//    auto args = f1.args();
//    std::cout << "Arguments: ";
//    for (const auto& arg : args) {
//        if (arg.type() == typeid(int)) {
//            std::cout << std::any_cast<int>(arg) << " ";
//        }
//    }
//    std::cout << "\n"; // 输出: 2 3
//
//    // 赋值测试
//    FunctionWrapper f10;
//    f10 = f1;
//    std::cout << "f1 == f10: " << (f1 == f10) << "\n"; // true
//}