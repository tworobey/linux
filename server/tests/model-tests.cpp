#include <catch2/catch_test_macros.hpp>
#include "../src/model.h"

TEST_CASE("Dog initial state", "[model]") {
    model::Dog dog(model::Dog::Id{0}, "test", {0.0, 0.0});
    CHECK(dog.GetScore() == 0);
    CHECK(dog.GetBag().empty());
    CHECK(dog.GetPosition().x == 0.0);
    CHECK(dog.GetPosition().y == 0.0);
}

TEST_CASE("Dog bag operations", "[model]") {
    model::Dog dog(model::Dog::Id{0}, "test", {0.0, 0.0});
    
    CHECK(dog.AddToBag({1, 0}, 3));
    CHECK(dog.AddToBag({2, 1}, 3));
    CHECK(dog.GetBag().size() == 2);
    
    // Рюкзак полон при capacity=2
    CHECK_FALSE(dog.AddToBag({3, 0}, 2));
    
    auto items = dog.EmptyBag();
    CHECK(items.size() == 2);
    CHECK(dog.GetBag().empty());
}

TEST_CASE("Dog score", "[model]") {
    model::Dog dog(model::Dog::Id{0}, "test", {0.0, 0.0});
    dog.AddScore(10);
    dog.AddScore(5);
    CHECK(dog.GetScore() == 15);
}
