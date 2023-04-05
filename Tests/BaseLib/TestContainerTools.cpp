/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <vector>

#include "BaseLib/ContainerTools.h"

template <typename T>
using PRACView = BaseLib::PolymorphicRandomAccessContainerView<T>;

namespace
{
struct Animal
{
    virtual unsigned legs() const = 0;

    virtual ~Animal() = default;

    std::string color = "transparent";
};

struct Pet : Animal
{
    virtual std::string name() const = 0;
};

struct Budgie final : Pet
{
    unsigned legs() const override { return 2; }
    std::string name() const override { return "Hansi"; }
};

struct Dog final : Pet
{
    unsigned legs() const override { return 4; }
    std::string name() const override { return "Bello"; }
};

void assertDogsNames(auto const& dogs)
{
    ASSERT_EQ(2, dogs.size());
    EXPECT_EQ("Bello", dogs[0].name());
    EXPECT_EQ("Bello", dogs[1].name());
}

void assertDogsLegs(auto const& dogs)
{
    ASSERT_EQ(2, dogs.size());
    EXPECT_EQ(4, dogs[0].legs());
    EXPECT_EQ(4, dogs[1].legs());
}

void assertPetsNames(auto const& pets)
{
    ASSERT_EQ(2, pets.size());
    EXPECT_EQ("Hansi", pets[0].name());
    EXPECT_EQ("Bello", pets[1].name());
}

void assertPetsLegs(auto const& pets)
{
    ASSERT_EQ(2, pets.size());
    EXPECT_EQ(2, pets[0].legs());
    EXPECT_EQ(4, pets[1].legs());
}
}  // namespace

TEST(BaseLib, ContainerToolsIteratorAndRangeConcepts)
{
    std::vector<Dog> data;
    PRACView<Dog> const view{data};

    using IteratorType = decltype(view.begin());
    static_assert(std::input_iterator<IteratorType>);
    static_assert(std::forward_iterator<IteratorType>);
    static_assert(std::bidirectional_iterator<IteratorType>);
    static_assert(std::random_access_iterator<IteratorType>);
}

TEST(BaseLib, ContainerToolsNoUpCast)
{
    std::vector<Dog> dogs{{}, {}};

    {
        // Dog -> Dog
        auto f = [](PRACView<Dog> const& dogs_) { assertDogsNames(dogs_); };

        // pass mutable vector
        f(dogs);

        // pass const vector - does not compile
        /*
        auto const& dogs_const = dogs;
        f(dogs_const);
        */
    }

    {
        // Dog -> const Dog
        auto f = [](PRACView<const Dog> const& dogs_)
        { assertDogsNames(dogs_); };

        // pass mutable vector
        f(dogs);

        // pass const vector
        auto const& dogs_const = dogs;
        f(dogs_const);
    }
}

TEST(BaseLib, ContainerToolsUpCast)
{
    std::vector<Dog> dogs{{}, {}};

    {
        // Dog -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        { assertDogsLegs(animals_); };

        // pass mutable vector
        f(dogs);

        // pass const vector - does not compile
        /*
        auto const& dogs_const = dogs;
        f(dogs_const);
        */
    }

    {
        // Dog -> const Animal
        auto f = [](PRACView<const Animal> const& animals_)
        { assertDogsLegs(animals_); };

        // pass mutable vector
        f(dogs);

        // pass const vector
        auto const& dogs_const = dogs;
        f(dogs_const);
    }
}

TEST(BaseLib, ContainerToolsUniquePtrNoUpCast)
{
    std::vector<std::unique_ptr<Pet>> pets;
    pets.push_back(std::make_unique<Budgie>());
    pets.push_back(std::make_unique<Dog>());

    {
        // Pet -> Pet
        auto f = [](PRACView<Pet> const& pets_) { assertPetsNames(pets_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }

    {
        // Pet -> const Pet
        auto f = [](PRACView<const Pet> const& pets_)
        { assertPetsNames(pets_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }
}

TEST(BaseLib, ContainerToolsUniquePtrUpCast)
{
    std::vector<std::unique_ptr<Pet>> pets;
    pets.push_back(std::make_unique<Budgie>());
    pets.push_back(std::make_unique<Dog>());

    {
        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        { assertPetsLegs(animals_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }

    {
        // Pet -> const Animal
        auto f = [](PRACView<const Animal> const& animals_)
        { assertPetsLegs(animals_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }
}

TEST(BaseLib, ContainerToolsUniquePtrConstNoUpCast)
{
    std::vector<std::unique_ptr<const Pet>> pets;
    pets.push_back(std::make_unique<Budgie>());
    pets.push_back(std::make_unique<Dog>());

    {
        // const Pet -> const Pet
        auto f = [](PRACView<const Pet> const& pets_)
        { assertPetsNames(pets_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }
}

TEST(BaseLib, ContainerToolsUniquePtrConstUpCast)
{
    std::vector<std::unique_ptr<const Pet>> pets;
    pets.push_back(std::make_unique<Budgie>());
    pets.push_back(std::make_unique<Dog>());

    {
        // const Pet -> const Animal
        auto f = [](PRACView<const Animal> const& animals_)
        { assertPetsLegs(animals_); };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }
}

TEST(BaseLib, ContainerToolsRangeFor)
{
    {
        std::vector<std::unique_ptr<Pet>> pets;
        pets.push_back(std::make_unique<Budgie>());
        pets.push_back(std::make_unique<Dog>());

        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            for (auto& animal : animals_)
            {
                EXPECT_LE(2, animal.legs());
                EXPECT_GE(4, animal.legs());
            }
        };

        // pass mutable vector
        f(pets);

        // pass const vector
        auto const& pets_const = pets;
        f(pets_const);
    }

    {
        std::vector<Dog> dogs{{}, {}};

        // Dog -> const Animal
        auto f = [](PRACView<const Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            for (auto const& animal : animals_)
            {
                EXPECT_EQ(4, animal.legs());
            }
        };

        // pass mutable vector
        f(dogs);

        // pass const vector
        auto const& dogs_const = dogs;
        f(dogs_const);
    }
}

TEST(BaseLib, ContainerToolsAlgorithms)
{
    std::vector<std::unique_ptr<Pet>> pets;
    pets.push_back(std::make_unique<Budgie>());
    pets.push_back(std::make_unique<Dog>());

    PRACView<Animal> animals{pets};
    ASSERT_EQ(2, animals[0].legs());
    ASSERT_EQ(4, animals[1].legs());

    auto const total_legs =
        std::accumulate(std::begin(animals), std::end(animals), 0,
                        [](auto const legs, Animal const& animal)
                        { return legs + animal.legs(); });
    ASSERT_EQ(6, total_legs);
}

TEST(BaseLib, ContainerToolsModify)
{
    // modify via range-for-loop
    {
        std::vector<Dog> dogs{{}, {}};

        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            // modify via range-for-loop
            for (auto& animal : animals_)
            {
                animal.color = "red";
            }
        };

        // pass mutable vector
        f(dogs);

        for (auto& dog : dogs)
        {
            EXPECT_EQ("red", dog.color);
        }
    }

    // modify directly
    {
        std::vector<Dog> dogs{{}, {}};

        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            // modify directly
            animals_[0].color = "yellow";
            animals_[1].color = "green";
        };

        std::span<Dog> dogs_span{dogs};  // spans work, too
        f(dogs_span);

        EXPECT_EQ("yellow", dogs[0].color);
        EXPECT_EQ("green", dogs[1].color);
    }
}

TEST(BaseLib, ContainerToolsUniquePtrModify)
{
    // modify via range-for-loop
    {
        // works with arrays and shared pointers, too
        std::array<std::shared_ptr<Pet>, 2> pets;
        pets[0] = std::make_shared<Budgie>();
        pets[1] = std::make_shared<Dog>();

        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            // modify via range-for-loop
            for (auto& animal : animals_)
            {
                animal.color = "red";
            }
        };

        // pass mutable vector
        f(pets);

        for (auto& pet : pets)
        {
            EXPECT_EQ("red", pet->color);
            pet->color = "blue";
        }

        // pass const vector - modification possible, because vector contains
        // unique_ptrs
        auto const& pets_const = pets;
        f(pets_const);

        for (auto& pet : pets)
        {
            EXPECT_EQ("red", pet->color);
        }
    }

    // modify directly
    {
        std::vector<std::unique_ptr<Pet>> pets;
        pets.push_back(std::make_unique<Budgie>());
        pets.push_back(std::make_unique<Dog>());

        // Pet -> Animal
        auto f = [](PRACView<Animal> const& animals_)
        {
            ASSERT_EQ(2, animals_.size());

            // modify directly
            animals_[0].color = "yellow";
            animals_[1].color = "green";
        };

        // pass mutable vector
        f(pets);

        EXPECT_EQ("yellow", pets[0]->color);
        EXPECT_EQ("green", pets[1]->color);

        for (auto& pet : pets)
        {
            pet->color = "blue";
        }

        // pass const vector - modification possible, because vector contains
        // unique_ptrs
        auto const& pets_const = pets;
        f(pets_const);

        EXPECT_EQ("yellow", pets[0]->color);
        EXPECT_EQ("green", pets[1]->color);
    }
}
