# value_ptr
A value semantics smart pointer for C++.

## Index

- [Introduction](#introduction)
  - [Disclaimer](#disclaimer)
- [Features](#features)
- [The Code](#the-code)
  - [Supporting Definitions](#supporting-definitions)
    - [`condition`](#condition)
    - [`is_cloneable`](#is-cloneable)

* * *

## Introduction

This templated class (ie. `value_ptr`) picks up where [proposal N3339](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf) left off and fleshes out a value semantics smart pointer (ie. a smart pointer that will copy, move and delete itself as if it were a value).

### Disclaimer

I'm by no means a template metaprogramming expert, and these sort of things can get pretty difficult and unwieldy, especially in these sort of settings, so feel free to criticize, correct, and enhance the code hereby provided, and if you have a better grasp of the techniques involved (quite possible, actually), please _do_ submit an issue / pull request.

## Features

The `value_ptr` class exhibits the following features:

- automatic determination of replication strategy (either a `clone` method or a copy constructor),
- initialization from other smart pointer classes:
  - by copy / move of `auto_ptr` and `unique_ptr`,
  - by copy of `shared_ptr` and `weak_ptr`,
- full `swap` support,
- full comparison support (ie. `operator==`, `operator!=`, `operator<`, `operator>`, `operator<=`, `operator>=`) based on pointer values,
- safe-bool conversion.

## The Code

Here we present an in depth analysis of the code provided. Much of this information may be found in the code comments themselves, but are reproduced here for convenience.

### Supporting Definitions

There are a number of supporting definitions in the provided source code. Here we treat each in turn.

#### `condition`

The template type `condition` is defined thus:

````c++
template <bool C>
using condition = std::conditional<C, std::true_type, std::false_type>;
````

It is simply a shorthand in place solely for convenience.

#### `is_cloneable`

The metaprogramming predicate `is_cloneable` is the workhorse used to determine whether we can use a `clone` method or we need to resort to copy construction. It is defined thus:

````c++
template <typename T>
struct is_cloneable {
  protected:
    template <typename>
    static constexpr auto test(...) -> std::false_type;

    template <typename S>
    static constexpr auto test(decltype(&S::clone))
      -> decltype(test(&S::clone, nullptr));

    template <typename S, typename R, typename ...U>
    static constexpr auto test(R *(S::*)(U...) const, nullptr_t)
      -> typename condition<
        sizeof(R) == sizeof(S) &&
        std::is_base_of<R, S>::value &&
        std::is_same<R *, decltype(std::declval<S>().clone())>::value
      >::type;

  public:
    static constexpr bool value = std::is_polymorphic<T>::value && decltype(test<T>(nullptr))::value;
};
````

and it's not your typical member detector.

Before we dwell into how this works, let's define precisely what a `clone` method should look like.

A `clone` method is a [virtual constructor](https://isocpp.org/wiki/faq/virtual-functions#virtual-ctors), it allows one to make a copy of a _derived_ object through a pointer to a _base_, in other words: it polymorphically copies an object. You can read all about virtual constructors in the link provided, but a simple snippet illustrating the idea would be:

````c++
class Base {
  public:
    virtual Base *clone() const { return new Base(*this); }
};

class Derived : public Base {
  public:
    virtual Derived *clone() const override { return new Derived(*this); }
};

````

now you can do:

````c++
Base *bp = new Derived();
...
Base *bp2 = bp->clone();
````

and end up having `bp2` point to a `Derived` object. That's really all there is to it, but you're still very much recommended to go check that link up.

Now for a method named `clone` to be a `clone` method it needs to satisfy the following (ideal) conditions:

- it must be named `clone` (duh),
- it mutt be `const`,
- it must either have no parameters or provide default arguments for them all,
- it must return a pointer to a type that is either that of the class in question or a base thereof,
- the size of the pointed-to object returned must be the same as the size of the class in question,
- it must be virtual.

Note that we can have a method satisfying all of the above conditions and _still_ it not be a `clone` method as we're likely to interpret it, alas, this is unavoidable.

Furthermore, I don't know of a bulletproof way of determining whether a method is virtual within a class, so that throws our last condition above out the window.

Finally, the next-to-last condition must sound really weird, but there's a particularly ugly set of conditions that can illustrate its necessity. Suppose we have the following classes:

````c++
class A {
  int anInt;
  public:
    virtual A *clone() const { return new A(*this); }
};

class B : public A {};
````

Now `B` inherits from `A`, since `B` adds no members to `A`'s ones there's no need for `B` to override `A`'s `clone` method (suppose `B` merely adds some semantically-safe functionality to `A`). The inherited `clone` method still has the signature:

````c++
virtual A *clone() const;
````

but since our fourth condition above allows this, we can safely say the `B`'s `clone` is indeed a `clone` method.

Everything looks peachy so far, but now some soulless monster goes on and adds:

````c++
class C : public B {
  int anotherInt;
};
````

As before, `C` inherits `B`'s `clone`, which has the signature:

````c++
virtual A *clone() const;
````

once more, but now we're in trouble: `C`'s `clone` will effectively call `A`'s, and `A`'s copy constructor knows nothing about `anotherInt`, and now we would _slice_ whenever we cloned (the very effect we wanted to avoid in the first place).

Our next-to-last condition takes care of this.

Of course, this only means that the _object itself_  won't be sliced, it does not mean that we'd be doing anything particularly meaningful all the time. Consider the following example:

````c++
class A {
  public:
    virtual A *clone() const { return new A(*this); }
};

class B : public A {
  static std::size_t instanceCount;
  public:
    B(B const &other) : A(other) { instanceCount++; }
    virtual ~B() noexcept { instanceCount--; }
};

B::instanceCount = 0;
````

In this case, by forgetting to define a `clone` method for `B`, we're losing the opportunity to count instances when cloning; our trait above would still detect `B` to be a cloneable class (one can do so without slicing), but it would not be semantically sound to do so. Yet, checking for this kind of cases seems impossible, and strengthening the predicate to require more stringent conditions than necessary would hurt more than it would help.

> One way to ensure a consistent `clone` behavior would be to define:
>
> ````c++
> template <typename T>
> class Cloneable {
>   public:
>     virtual T *clone() const { return new T(*this); }
> };
> ````
>
> One would then use it by applying the [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) thus:
>
> ````c++
> class Whatever : public Cloneable<Whatever> {
>   ...
> };
> ````
> And forget about every little nuance about cloning in 99% of the cases, while still allowing you to override the template-defined `clone` if needed. Note though, that this may imply that you'll need multiple inheritance every now and then, and some "coding standards" feel very strongly against that (which I find hilarious).

So, that explains two of the six conditions above, the rest are "easy"-ish to check in a template trait and should come as no surprise.

On to the nitty-gritty details of how it works.

The only accessible member of `is_cloneable` is the `value` static member.
