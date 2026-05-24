#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/model.h"
#include "../src/model_serialization.h"

using namespace model;
using namespace std::literals;

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

} // namespace

SCENARIO_METHOD(Fixture, "DogPosition serialization") {
    GIVEN("A position") {
        DogPosition pos{10.5, 20.3};
        WHEN("position is serialized") {
            output_archive << pos;
            THEN("it is equal after deserialization") {
                InputArchive input_archive{strm};
                DogPosition restored;
                input_archive >> restored;
                CHECK(pos.x == restored.x);
                CHECK(pos.y == restored.y);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog serialization") {
    GIVEN("a dog with bag and score") {
        Dog dog(Dog::Id{42}, "Pluto"s, {42.2, 12.5});
        dog.AddScore(100);
        dog.AddToBag({1, 2}, 10);
        dog.SetDirection(DogDirection::EAST);
        dog.SetSpeed({2.3, -1.2});

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }
            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(*dog.GetId() == *restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog.GetPosition().x == restored.GetPosition().x);
                CHECK(dog.GetPosition().y == restored.GetPosition().y);
                CHECK(dog.GetScore() == restored.GetScore());
                CHECK(dog.GetBag().size() == restored.GetBag().size());
            }
        }
    }
}
