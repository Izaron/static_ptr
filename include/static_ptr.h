#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

namespace sp {

namespace _ {

// functors
template <typename T>
struct move_constructer {
    static void call(T* lhs, T* rhs)
        noexcept (std::is_nothrow_move_constructible_v<T>)
        requires (std::is_move_constructible_v<T>)
    {
        new (lhs) T(std::move(*rhs));
    }

    static void call(T* lhs, T* rhs)
        noexcept (std::is_nothrow_default_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
        requires (!std::is_move_constructible_v<T> && std::is_default_constructible_v<T> && std::is_move_assignable_v<T>)
    {
        new (lhs) T();
        *lhs = std::move(*rhs);
    }
};

template<typename T>
struct move_assigner {
    static void call(T* lhs, T* rhs)
        noexcept (std::is_nothrow_move_assignable_v<T>)
        requires (std::is_move_assignable_v<T>)
    {
        *lhs = std::move(*rhs);
    }

    static void call(T* lhs, T* rhs)
        noexcept (std::is_nothrow_move_constructible_v<T>)
        requires (!std::is_move_assignable_v<T> && std::is_move_constructible_v<T>)
    {
        lhs->~T();
        new (lhs) T(std::move(*rhs));
    }
};

// ops struct definition
struct ops {
    using binary_func = void(*)(void* dst, void* src);
    using unary_func = void(*)(void* dst);

    binary_func move_construct_func;
    binary_func move_assign_func;
    unary_func destruct_func;
};

template<typename T, typename Functor>
void call_typed_func(void* dst, void* src) {
    Functor::call(static_cast<T*>(dst), static_cast<T*>(src));
}

template<typename T>
void destruct_func(void* dst) {
    static_cast<T*>(dst)->~T();
}

template<typename T>
constexpr ops ops_for{
    .move_construct_func = &call_typed_func<T, move_constructer<T>>,
    .move_assign_func = &call_typed_func<T, move_assigner<T>>,
    .destruct_func = &destruct_func<T>,
};
using ops_ptr = const ops*;

// moving objects using ops
static void move_construct(void* dst_buf, ops_ptr& dst_ops,
                           void* src_buf, ops_ptr& src_ops) {
    if (!src_ops && !dst_ops) {
        // both object are nullptr_t, do nothing
        return;
    } else if (src_ops == dst_ops) {
        // objects have the same type, make move
        (*src_ops->move_assign_func)(dst_buf, src_buf);
        (*src_ops->destruct_func)(src_buf);
        src_ops = nullptr;
    } else {
        // objects have different type
        // delete the old object
        if (dst_ops) {
            (*dst_ops->destruct_func)(dst_buf);
            dst_ops = nullptr;
        }
        // construct the new object
        if (src_ops) {
            (*src_ops->move_construct_func)(dst_buf, src_buf);
            (*src_ops->destruct_func)(src_buf);
        }
        dst_ops = src_ops;
        src_ops = nullptr;
    }
}

} // namespace _

// static_ptr traits struct
template<typename T>
struct static_ptr_traits {
    static constexpr std::size_t buffer_size = std::max(static_cast<std::size_t>(16), sizeof(T));
};

template<typename Base>
requires(!std::is_void_v<Base>)
class static_ptr {
private:
    static constexpr std::size_t buffer_size = static_ptr_traits<Base>::buffer_size;
    static constexpr std::size_t align = alignof(std::max_align_t);

    // support static_ptr's conversions of different types
    template <typename T> friend class static_ptr;

    // Struct for calling object's operators
    // equals to `nullptr` when `buf_` contains no object
    // equals to `ops_for<T>` when `buf_` contains a `T` object
    _::ops_ptr ops_;

    // Storage for underlying `T` object
    // this is mutable so that `operator*` and `get()` can
    // be marked const
    mutable std::aligned_storage_t<buffer_size, align> buf_;

    template<typename Derived>
    struct derived_class_check {
        static constexpr bool ok = sizeof(Derived) <= buffer_size && std::is_base_of_v<Base, Derived>;
    };

public:
    // operators, ctors, dtor
    static_ptr() noexcept : ops_{nullptr} {}

    static_ptr(std::nullptr_t) noexcept : ops_{nullptr} {}
    static_ptr& operator=(std::nullptr_t) noexcept(std::is_nothrow_destructible_v<Base>) {
        reset();
        return *this;
    }

    template<typename Derived = Base>
    static_ptr(static_ptr<Derived>&& rhs)
        requires(derived_class_check<Derived>::ok)
        : ops_{nullptr}
    {
        _::move_construct(&buf_, ops_, &rhs.buf_, rhs.ops_);
    }

    template<typename Derived = Base>
    static_ptr& operator=(static_ptr<Derived>&& rhs)
        requires(derived_class_check<Derived>::ok)
    {
        _::move_construct(&buf_, ops_, &rhs.buf_, rhs.ops_);
        return *this;
    }

    static_ptr(const static_ptr&) = delete;
    static_ptr& operator=(const static_ptr&) = delete;

    ~static_ptr() {
        reset();
    }

    // in-place (re)initialization
    template<typename Derived = Base, typename ...Args>
    Derived& emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Derived, Args...>)
        requires(derived_class_check<Derived>::ok)
    {
        reset();
        Derived* derived = new (&buf_) Derived(std::forward<Args>(args)...);
        ops_ = &_::ops_for<Derived>;
        return *derived;
    }

    // destruct the underlying object
    void reset() noexcept(std::is_nothrow_destructible_v<Base>) {
        if (ops_) {
            (ops_->destruct_func)(&buf_);
            ops_ = nullptr;
        }
    }

    // accessors
    Base* get() noexcept {
        return ops_ ? reinterpret_cast<Base*>(&buf_) : nullptr;
    }
    const Base* get() const noexcept {
        return ops_ ? reinterpret_cast<const Base*>(&buf_) : nullptr;
    }

    Base& operator*() noexcept { return *get(); }
    const Base& operator*() const noexcept { return *get(); }

    Base* operator&() noexcept { return get(); }
    const Base* operator&() const noexcept { return get(); }

    Base* operator->() noexcept { return get(); }
    const Base* operator->() const noexcept { return get(); }

    operator bool() const noexcept { return ops_; }
};

template<typename T, class ...Args>
static static_ptr<T> make_static(Args&&... args) {
    static_ptr<T> ptr;
    ptr.emplace(std::forward<Args>(args)...);
    return ptr;
}

} // namespace sp

#define STATIC_PTR_BUFFER_SIZE(Tp, size)                   \
namespace sp {                                             \
    template<> struct static_ptr_traits<Tp> {              \
        static constexpr std::size_t buffer_size = size;   \
    };                                                     \
}

#define STATIC_PTR_INHERITED_BUFFER_SIZE(Tp, size)         \
namespace sp {                                             \
    template<typename T> requires std::is_base_of_v<Tp, T> \
    struct static_ptr_traits<T> {                          \
        static constexpr std::size_t buffer_size = size;   \
    };                                                     \
}
