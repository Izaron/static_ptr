#include "static_ptr.h"
#include <gtest/gtest.h>
#include <vector>

namespace {

using EventsType = std::vector<std::string>;

class IEngine {
public:
    IEngine(EventsType& events) : Events_{events} {}
    const EventsType& Events() const { return Events_; }

    virtual ~IEngine() = default;
    virtual void Do() = 0;

protected:
    EventsType& Events_;
};

class TSteamEngine : public IEngine {
public:
    TSteamEngine(EventsType& events) : IEngine{events} {
        Events_.push_back("TSteamEngine::TSteamEngine()");
    }

    TSteamEngine(TSteamEngine&& engine) : IEngine{engine} {
        Events_.push_back("TSteamEngine::TSteamEngine(TSteamEngine&&)");
    }

    TSteamEngine& operator=(TSteamEngine&& engine) {
        Events_.push_back("operator=(TSteamEngine&&)");
        return *this;
    }

    void Do() override {
        Events_.push_back("TSteamEngine::Do()");
    }

    ~TSteamEngine() {
        Events_.push_back("TSteamEngine::~TSteamEngine()");
    }
};

class TJetEngine : public IEngine {
public:
    TJetEngine(EventsType& events) : IEngine{events} {
        Events_.push_back("TJetEngine::TJetEngine()");
    }

    TJetEngine(TJetEngine&& engine) : IEngine{engine} {
        Events_.push_back("TJetEngine::TJetEngine(TJetEngine&&)");
    }

    TJetEngine& operator=(TJetEngine&& engine) {
        Events_.push_back("operator=(TJetEngine&&)");
        return *this;
    }

    void Do() override {
        Events_.push_back("TJetEngine::Do()");
    }

    ~TJetEngine() {
        Events_.push_back("TJetEngine::~TJetEngine()");
    }
};

} // namespace

TEST(Derived, WithoutStaticPtr) {
    EventsType events;
    {
        TSteamEngine engine{events};
        engine.Do();
    }

    const EventsType expected{
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, NoOpStaticPtr) {
    EventsType events;
    {
        auto engine = sp::make_static<TSteamEngine>(events);
        engine->Do();
    }

    const EventsType expected{
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, EmplaceBase) {
    EventsType events;
    {
        sp::static_ptr<IEngine> engine;
        EXPECT_TRUE(!engine);
        engine.emplace<TSteamEngine>(events);
        EXPECT_TRUE(engine);
        engine->Do();
    }

    const EventsType expected{
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, EmplaceNewObjectSameType) {
    EventsType events;
    {
        sp::static_ptr<IEngine> engine;
        EXPECT_TRUE(!engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) == nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) == nullptr);

        engine.emplace<TSteamEngine>(events);
        EXPECT_TRUE(engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) != nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) == nullptr);
        engine->Do();

        engine.emplace<TSteamEngine>(events);
        EXPECT_TRUE(engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) != nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) == nullptr);
        engine->Do();
    }

    const EventsType expected{
        // emplace steam engine
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
        // emplace steam engine again
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, EmplaceNewObjectChangeType) {
    EventsType events;
    {
        sp::static_ptr<IEngine> engine;
        EXPECT_TRUE(!engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) == nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) == nullptr);

        engine.emplace<TSteamEngine>(events);
        EXPECT_TRUE(engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) != nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) == nullptr);
        engine->Do();

        engine.emplace<TJetEngine>(events);
        EXPECT_TRUE(engine);
        EXPECT_TRUE(dynamic_cast<TSteamEngine*>(engine.get()) == nullptr);
        EXPECT_TRUE(dynamic_cast<TJetEngine*>(engine.get()) != nullptr);
        engine->Do();
    }

    const EventsType expected{
        // emplace steam engine
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
        // emplace jet engine
        "TJetEngine::TJetEngine()",
        "TJetEngine::Do()",
        "TJetEngine::~TJetEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, MoveAssignSameType) {
    EventsType events;
    {
        sp::static_ptr<IEngine> engine;
        engine.emplace<TSteamEngine>(events);
        engine->Do();
        engine = sp::make_static<TSteamEngine>(events);
        engine->Do();
    }

    const EventsType expected{
        // first engine's work
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        // second engine's creation and MOVE ASSIGN and deletion of tmp object
        "TSteamEngine::TSteamEngine()",
        "operator=(TSteamEngine&&)",
        "TSteamEngine::~TSteamEngine()",
        // second engine's work and deletion
        "TSteamEngine::Do()",
        "TSteamEngine::~TSteamEngine()",
    };
    EXPECT_EQ(events, expected);
}

TEST(Derived, MoveAssignChangeType) {
    EventsType events;
    {
        sp::static_ptr<IEngine> engine;
        engine.emplace<TSteamEngine>(events);
        engine->Do();
        engine = sp::make_static<TJetEngine>(events);
        engine->Do();
    }

    const EventsType expected{
        // first engine's work
        "TSteamEngine::TSteamEngine()",
        "TSteamEngine::Do()",
        // second engine's creation
        "TJetEngine::TJetEngine()",
        // first engine's deletion
        "TSteamEngine::~TSteamEngine()",
        // second engine's MOVE CONSTRUCTION and deletion of tmp object
        "TJetEngine::TJetEngine(TJetEngine&&)",
        "TJetEngine::~TJetEngine()",
        // second engine's work and deletion
        "TJetEngine::Do()",
        "TJetEngine::~TJetEngine()",
    };
    EXPECT_EQ(events, expected);
}
