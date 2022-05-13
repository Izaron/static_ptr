#include "static_ptr.h"
#include <gtest/gtest.h>

namespace {

// struct of size way bigger than 16
struct BigType {
    char arr[1024];
};

// virtual classes add `sizeof(void*)` to size because of vtable
struct Animal {
    virtual ~Animal() = default;
    char buffer1[32];
};

struct Cat : Animal {
    char buffer2[32];
};

// another pair of virtual classes
struct IEngine {
    virtual ~IEngine() = default;
    char buffer1[32];
};

struct TSteamEngine : IEngine {
    char buffer2[32];
};

// another pair of virtual classes
struct ILanguage {
    virtual ~ILanguage() = default;
    char buffer1[32];
};

struct TCxx : ILanguage {
    char buffer2[32];
};

// helper methods
template<typename Base, typename Derived>
consteval bool can_emplace() {
    return requires (sp::static_ptr<Base>& p) { p = sp::make_static<Derived>(); };
}

} // namespace

TEST(BufferSize, DefaultSize) {
    constexpr std::size_t default_buffer_size = 16;
    EXPECT_EQ(sp::static_ptr_traits<char>::buffer_size, default_buffer_size);
    EXPECT_EQ(sp::static_ptr_traits<int>::buffer_size, default_buffer_size);
    EXPECT_EQ(sp::static_ptr_traits<long double>::buffer_size, default_buffer_size);
}

TEST(BufferSize, BigTypeSize) {
    // if `sizeof(T)` is bigger than 16, the buffer size is `sizeof(T)`
    EXPECT_EQ(sp::static_ptr_traits<BigType>::buffer_size, sizeof(BigType));
}

TEST(BufferSize, VirtualTypeSize) {
    // derived class is bigger than base class, so are the buffer_sizes
    EXPECT_EQ(sp::static_ptr_traits<Animal>::buffer_size, sizeof(Animal));
    EXPECT_EQ(sp::static_ptr_traits<Cat>::buffer_size, sizeof(Cat));
    EXPECT_EQ(sizeof(Cat), sizeof(Animal) + 32 * sizeof(char));

    // can emplace itself
    EXPECT_TRUE((can_emplace<Animal, Animal>()));
    // can't emplace derived class that is bigger than buffer_size
    EXPECT_FALSE((can_emplace<Animal, Cat>()));
    // can't emplace non-derived class
    EXPECT_FALSE((can_emplace<Cat, Animal>()));
    EXPECT_FALSE((can_emplace<Animal, int>()));
    EXPECT_FALSE((can_emplace<Animal, void>()));
    EXPECT_FALSE((can_emplace<Animal, double>()));
    EXPECT_FALSE((can_emplace<Animal, sp::static_ptr<Animal>>()));
}

// RedefineBufferSizeSimple
STATIC_PTR_BUFFER_SIZE(IEngine, 1024)

TEST(BufferSize, RedefineBufferSizeSimple) {
    // derived class DOESN'T inherit buffer size
    EXPECT_EQ(sp::static_ptr_traits<IEngine>::buffer_size, 1024);
    EXPECT_EQ(sp::static_ptr_traits<TSteamEngine>::buffer_size, sizeof(TSteamEngine));

    // "real size" of derived class is bigger, but the static_ptr can hold derived class object
    EXPECT_TRUE(sizeof(IEngine) < sizeof(TSteamEngine));
    EXPECT_TRUE((can_emplace<IEngine, TSteamEngine>()));
}

// RedefineBufferSizeInherited
STATIC_PTR_INHERITED_BUFFER_SIZE(ILanguage, 1024)

TEST(BufferSize, RedefineBufferSizeInherited) {
    // derived class DO inherit buffer size
    EXPECT_EQ(sp::static_ptr_traits<ILanguage>::buffer_size, 1024);
    EXPECT_EQ(sp::static_ptr_traits<TCxx>::buffer_size, 1024);

    // "real size" of derived class is bigger, but the static_ptr can hold derived class object
    EXPECT_TRUE(sizeof(ILanguage) < sizeof(TCxx));
    EXPECT_TRUE((can_emplace<ILanguage, TCxx>()));
}
