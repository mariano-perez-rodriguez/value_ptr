# value_ptr
A value semantics smart pointer for C++.

## Index

- [Introduction](#introduction)
  - [Disclaimer](#disclaimer)
- [Features](#features)
  - [Answers to Questions Posed in Proposal N3339](#answers-to-questions-posed-in-proposal-n3339)
    - [Should `value_ptr` be specialized to work with array types à la unique_ptr?](#should-value-ptr-be-specialized-to-work-with-array-types-a-la-unique-ptr)
    - [Should `value_ptr` take an `allocator` argument in addition to a `replicator` and a `deleter`?](#should-value-ptr-take-an-allocator-argument-in-addition-to-a-replicator-and-a-deleter)
    - [This implementation assumes that the `replicator` and `deleter` types are stateless; are these viable assumptions? If not, what policies should apply when they are being copied during a `value_ptr` copy?](#this-implementation-assumes-that-the-replicator-and-deleter-types-are-stateless-are-these-viable-assumptions-if-not-what-policies-should-apply-when-they-are-being-copied-during-a-value_ptr-copy)
    - [With which, if any, standard smart pointers should this template innately interoperate, and to what degree?](#with-which-if-any-standard-smart-pointers-should-this-template-innately-interoperate-and-to-what-degree)
    - [What color should the bicycle shed be painted?](#what-color-should-the-bicycle-shed-be-painted)
- [The Code](#the-code)
  - [Supporting Definitions](#supporting-definitions)
    - [`condition`](#condition)
    - [`is_cloneable`](#is-cloneable)
    - [`default_copy`](#default-copy)
    - [`default_clone`](#default-clone)
    - [`default_replicate`](#default-replicate)
  - [The `value_ptr` Class](#the-value-ptr-class)
    - [`using` Declarations](#using-declarations)
    - [Constructors and Destructor](#constructors-and-destructor)
      - [Default, Copy, and Move Constructors](#default-copy-and-move-constructors)
      - [Templated Copy and Move Constructors](#templated-copy-and-move-constructors)
      - [Ownership Taking Constructors](#ownership-taking-constructors)
      - [`nullptr` Constructors](#nullptr-constructors)
      - [`auto_ptr` Constructors](#auto-ptr-constructors)
      - [`unique_ptr` Constructors](#unique-ptr-constructors)
      - [`shared_ptr` Constructors](#shared-ptr-constructors)
      - [`weak_ptr` Constructors](#weak-ptr-constructors)
    - [Assignments](#assignments)
      - [Copy and Move Assignment](#copy-and-move-assignment)
      - [`nullptr` Assignment](#nullptr-assignment)
      - [Templated Copy and Move Assignment](#templated-copy-and-move-assignment)
    - [Safe `bool` Conversion](#safe-bool-conversion)
    - [Observers](#observers)
      - [`operator*`](#)
      - [`operator->`](#)
      - [`get()`](#get)
      - [`get_replicator()`](#get-replicator)
      - [`get_deleter()`](#get-deleter)
    - [Modifiers](#modifiers)
      - [`release`](#release)
      - [`reset`](#reset)
      - [Templated `swap`](#templated-swap)
  - [Non Member Functions](#non-member-functions)
    - [Swap Overloads](#swap-overloads)
    - [Equality Overloads](#equality-overloads)
    - [`nullptr` Equality Overloads](#nullptr-equality-overloads)
    - [Comparison Overloads](#comparison-overloads)

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

### Answers to Questions Posed in Proposal N3339

In [proposal N3339](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf), a couple of questions were posed, here, we present our answers (so far) to them.

#### Should `value_ptr` be specialized to work with array types à la unique_ptr?

On the one hand, arrays are not _values_ themselves in ANSI C / ISO C++, but rather they decay into pointers, so, a priori, no, we should not.

On the other hand, this would be extremely useful.

The only drawback is that in order to properly clone an array, we must somehow know its size given just a pointer to it, and this can only be done in an ABI dependent manner, and, even then, not fort every possible type.
In the future, we may provide `T[]` specializations, but these will certainly be ABI dependent and fragile.

#### Should `value_ptr` take an `allocator` argument in addition to a `replicator` and a `deleter`?

Given that we implement stateful `replicator`s, there's no need for an additional `allocator` object: it can be provided on `replicator`'s initialization.

#### This implementation assumes that the `replicator` and `deleter` types are stateless; are these viable assumptions? If not, what policies should apply when they are being copied during a `value_ptr` copy?

Given that the standard smart pointers implement stateful deleters, this doesn't seem too viable an alternative, besides, we can swiftly solve the problem posed in the previous question by making them stateful.

This has been resolved similarly as to how `glibc++` does for `unique_ptr`, ie. with `std::tuple`.

#### With which, if any, standard smart pointers should this template innately interoperate, and to what degree?

The current `value_ptr` class interoperates with all the standard smart pointers for construction purposes (but see the section on `shared_ptr` and `weak_ptr` constructors for details).

These are the only cases in which they interoperate as of now.

#### What color should the bicycle shed be painted?

Most obviously, the color is fine.

## The Code

Here we present an in depth analysis of the code provided.
Much of this information may be found in the code comments themselves, but are reproduced here for convenience.

### Supporting Definitions

There are a number of supporting definitions in the provided source code.
Here we treat each in turn.

#### `condition`

The template type `condition` is defined thus:

````c++
template <bool C>
using condition = std::conditional<C, std::true_type, std::false_type>;
````

It is simply a shorthand in place solely for convenience.

#### `is_cloneable`

The metaprogramming predicate `is_cloneable` is the workhorse used to determine whether we can use a `clone` method or we need to resort to copy construction.
It is defined thus:

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

A `clone` method is a [virtual constructor](https://isocpp.org/wiki/faq/virtual-functions#virtual-ctors), it allows one to make a copy of a _derived_ object through a pointer to a _base_, in other words: it polymorphically copies an object.
You can read all about virtual constructors in the link provided, but a simple snippet illustrating the idea would be:

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

and end up having `bp2` point to a `Derived` object.
That's really all there is to it, but you're still very much recommended to go check that link up.

Now for a method named `clone` to be a `clone` method it needs to satisfy the following (ideal) conditions:

- it must be named `clone` (duh),
- it mutt be `const`,
- it must either have no parameters or provide default arguments for them all,
- it must return a pointer to a type that is either that of the class in question or a base thereof,
- the size of the pointed-to object returned must be the same as the size of the class in question,
- it must be virtual.

Note that we can have a method satisfying all of the above conditions and _still_ it not be a `clone` method as we're likely to interpret it, alas, this is unavoidable.

Furthermore, I don't know of a bulletproof way of determining whether a method is virtual within a class, so that throws our last condition above out the window.

Finally, the next-to-last condition must sound really weird, but there's a particularly ugly set of conditions that can illustrate its necessity.
Suppose we have the following classes:

````c++
class A {
  int anInt;
  public:
    virtual A *clone() const { return new A(*this); }
};

class B : public A {};
````

Now `B` inherits from `A`, since `B` adds no members to `A`'s ones there's no need for `B` to override `A`'s `clone` method (suppose `B` merely adds some semantically-safe functionality to `A`).
The inherited `clone` method still has the signature:

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

Of course, this only means that the _object itself_  won't be sliced, it does not mean that we'd be doing anything particularly meaningful all the time.
Consider the following example:

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

In this case, by forgetting to define a `clone` method for `B`, we're losing the opportunity to count instances when cloning; our trait above would still detect `B` to be a cloneable class (one can do so without slicing), but it would not be semantically sound to do so.
Yet, checking for this kind of cases seems impossible, and strengthening the predicate to require more stringent conditions than necessary would hurt more than it would help.

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
> And forget about every little nuance about cloning in 99% of the cases, while still allowing you to override the template-defined `clone` if needed.
Note though, that this may imply that you'll need multiple inheritance every now and then, and some "coding standards" feel very strongly against that (which I find hilarious).

So, that explains two of the six conditions above, the rest are "easy"-ish to check in a template trait and should come as no surprise.

On to the nitty-gritty details of how it works.

The only accessible member of `is_cloneable` is the `value` static member, which is defined as the conjunction of `std::is_polymorphic<T>::value` (because it makes no sense to ask whether a non-polymorphic type is cloneable) and `decltype(test<T>(nullptr))::value`.
Now this last one is where the magic happens.
When trying to get the return type of `test<T>(nullptr)`, the compiler has two options:

- trying to substitute for `template <typename> static constexpr auto test(...) -> std::false_type`, or
- trying to substitute for `template <typename S> static constexpr auto test(decltype(&S::clone)) -> decltype(test(&S::clone, nullptr))`

The first is a method accepting any kind of argument whatsoever and returning `std::false_type`, this substitution is always viable.

The second is a method accepting a _single_ argument of type `decltype(&S::clone)` and returning a `decltype(test(&S::clone, nullptr))`.
Now substitution will only be viable if both `&S::clone` and `test(&S::clone, nullptr)` are themselves viable.
In the case of `&S::clone`, this will be viable only if the type `T` has a (possibly inherited) method named `clone`.
In the case of `test(&S::clone, nullptr)`, its viability and actual type will depend (see below).
If _any_ of these conditions doesn't hold, this whole substitution is not viable and we default to the first case above.

Now on to the `test(&S::clone, nullptr)` case.
Here, once more, the compiler has two options:

- trying to substitute for `template <typename> static constexpr auto test(...) -> std::false_type`, or
- trying to substitute for `template <typename S, typename R, typename ...U> static constexpr auto test(R *(S::*)(U...) const, nullptr_t) -> typename condition<sizeof(R) == sizeof(S) && std::is_base_of<R, S>::value && std::is_same<R *, decltype(std::declval<S>().clone())>::value>::type`

The first one is as before, since it is always viable.

The second one is a mouthful, but it can be analyzed as follows:

- first, try to match `&S::clone` to `R *(S::*)(U...) const`, that is, try to look for a type `R` and a parameter pack `...U` such that `T`'s `clone` method is a `const` method of `T` returning a pointer to `R` and accepting a parameter pack `U...` (we'll later see the use for this parameter pack, for now, since it applies no conditions to it, this simply means that `T`'s `clone` can take any number and type of parameters),
- second, analyze the following conditions, if they're _all_ true, the return type is `std::true_type`, otherwise, it is `std::false_type`:
  - the size of the pointed-to object returned by `T`'s `clone` must be the same as `T`'s,
  - the pointed-to type must either be a base of `T` or `T` itself,
  - all the arguments in the pack must be optional; this is perhaps the trickiest one to see, but it amounts to verifying that the return type deduced for the `clone` method so far is the same as the return type of a call to `std::declval<S>().clone()` (`std::declval<S>()` returns a value of type `S` only suitable for use on unevaluated contexts), ie. calling `clone` on an hypothetical object without passing any arguments.

Note the `nullptr_t` second parameter: this is here solely to disambiguate and make substitution into the previous case impossible.

Now, if any of those conditions has no viable substitution, the default case will kick in an we end up with a `std::false_type` in `value`.
If all of those conditions are found to be viable, their actual value will determine the resulting type.

Note that we've exhausted all of our implementable conditions, leaving the result in the `value` static member.

This metaprogramming trait is a modified version of [this](http://stackoverflow.com/a/10707822) absolutely marvelous StackOverflow answer by [Mike Kinghan](http://stackoverflow.com/users/1362568/mike-kinghan).

#### `default_copy`

The `default_copy` templated structure implements a replicator based off of the copy constructor of an underlying type.
It is defined as:

````c++
template <typename T>
struct default_copy {
  static_assert(std::is_copy_constructible<T>::value, "default_copy requires a copy constructor");

  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U> const &) noexcept {}
  template <typename U> default_copy(default_copy<U> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U> const &) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U> &&) noexcept {}
  virtual ~default_copy() noexcept {};

  T *operator()(T const *p) const { return nullptr != p ? new T{*p} : nullptr; }
};
````

We can see three main parts in it:

- the `static_assert` is there so that one doesn't try to ask for a `default_copy` on a non-copyable class, nothing more,
- the "boilerplate block", defining:
  - a default constructor,
  - templated copy constructor,
  - templated move constructor,
  - templated copy-assignment operator,
  - templated move-assignment operator, and
  - virtual destructor

  it's there just so that we can juggle around these objects without much hassle, and
- the `operator()`, which basically just calls the copy constructor on `T` unless given a `nullptr`, in which case it just returns a `nullptr` itself.

That's all there is to it.

#### `default_clone`

The `default_clone` templated structure implements a replicator based off of the `clone` method of an underlying type.
It is defined as:

````c++
template <typename T>
struct default_clone {
  static_assert(is_cloneable<T>::value, "default_clone requires a cloneable type");

  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U> const &) noexcept {}
  template <typename U> default_clone(default_clone<U> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U> const &) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U> &&) noexcept {}
  virtual ~default_clone() noexcept {};

  T *operator()(T const *p) const { return nullptr != p ? p->clone() : nullptr; }
};
````

We can see three main parts in it:

- the `static_assert` is there so that one doesn't try to ask for a `default_clone` on a non-cloneable class, nothing more,
- the "boilerplate block", defining:
  - a default constructor,
  - templated copy constructor,
  - templated move constructor,
  - templated copy-assignment operator,
  - templated move-assignment operator, and
  - virtual destructor

  it's there just so that we can juggle around these objects without much hassle, and
- the `operator()`, which basically just calls the `clone` method unless given a `nullptr`, in which case it just returns a `nullptr` itself.

That's all there is to it.

#### `default_replicate`

The `default_replicate` templated structure simply selects a default replication strategy for a given type based on its "cloneability".
It is defined as:

````c++
template <typename T, bool use_clone = is_cloneable<T>::value>
struct default_replicate;
````

and the following partial specializations provided:

````c++
template <typename T>
struct default_replicate<T, true> : public default_clone<T> {
  using default_clone<T>::operator();

  default_replicate() noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> &&) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> &&) noexcept {}
  virtual ~default_replicate() noexcept {};
};

template <typename T>
struct default_replicate<T, false> : public default_copy<T> {
  using default_copy<T>::operator();

  default_replicate() noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> &&) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> &&) noexcept {}
  virtual ~default_replicate() noexcept {};
};
````

These specializations either derive from `default_clone` (for the `true` specialization) or `default_copy` (for the `false` one).

We can see two main parts in each specialization itself:

- the `using` declaration merely brings into scope the `operator()` inherited in each case, and
- the "boilerplate block", defining:
  - a default constructor,
  - templated copy constructor,
  - templated move constructor,
  - templated copy-assignment operator,
  - templated move-assignment operator, and
  - virtual destructor

  it's there just so that we can juggle around these objects without much hassle.

Because of the default template argument declaration provided first, each partial specialization will be selected automatically as soon as it's "cloneability" is determined.

That's all there is to it.

### The `value_ptr` Class

Now we get to the meat of the matter.
The `value_ptr`class seems _massive_, but upon closer inspection 90% of it consists of constructor definitions.

We will treat each "part" of the class in turn.

#### `using` Declarations

The `value_ptr` class provides a couple of `using` declarations for convenience's sake.

The element types directives are:

````c++
using element_type   = T;
using pointer_type   = element_type *;
using reference_type = element_type &;
````

These expose simple type names for the element type and its alterations.

The replicator and deleter ones are similar to each other:

````c++
using replicator_type            = TRep;
using replicator_reference       = typename std::add_lvalue_reference<replicator_type>::type;
using replicator_const_reference = typename std::add_lvalue_reference<typename std::add_const<replicator_type>::type>::type;

using deleter_type            = TDel;
using deleter_reference       = typename std::add_lvalue_reference<deleter_type>::type;
using deleter_const_reference = typename std::add_lvalue_reference<typename std::add_const<deleter_type>::type>::type;
````

Finally, some `protected` declarations are in place as well:

````c++
using tuple_type = std::tuple<pointer_type, replicator_type, deleter_type>;
````

This defines the internal state used.

````c++
using __unspecified_bool_type = tuple_type value_ptr::*;
````

This one is used in the safe `bool` conversion operator.

````c++
template <typename U, typename V = nullptr_t>
using enable_if_compatible = std::enable_if<std::is_convertible<typename std::add_pointer<U>::type, pointer_type>::value, V>;
````

And this templated `using` specializes the `std::enable_if` trait to check for pointer compatibility.

#### Constructors and Destructor

We might as well start with `value_ptr`'s destructor, this is simply:

````c++
virtual ~value_ptr() noexcept { reset(); }
````

where `reset()` behaves as `unique_ptr`'s `reset()`.

Now, on to the constructors proper.

All of the constructors defined delegate construction to a protected "master" constructor, either directly or indirectly.
The protected "master" constructor's code is:

````c++
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(T2 *p, TRep2&& replicator, TDel2&& deleter, typename enable_if_compatible<T2>::type) noexcept : c{p, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {
  static_assert(!std::is_polymorphic<T>::value || !std::is_same<TRep, default_replicate<T, false>>::value, "would slice when copying");
  static_assert(!std::is_pointer<replicator_type>::value || !std::is_same<TRep2, nullptr_t>::value, "constructed with null function pointer replicator");
  static_assert(!std::is_pointer<deleter_type>::value || !std::is_same<TDel2, nullptr_t>::value, "constructed with null function pointer deleter");
  static_assert(!std::is_reference<replicator_type>::value || !std::is_rvalue_reference<TRep2>::value, "rvalue replicator bound to reference");
  static_assert(!std::is_reference<deleter_type>::value || !std::is_rvalue_reference<TDel2>::value, "rvalue replicator bound to reference");
}
````

Note the [universal references](https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers) on the replicator and deleter parameters: these accept _any_ qualification of the given types.

This "master" constructor checks that (remember that p → q ≡ ¬p ∨ q):

- if the type is polymorphic, the provided replicator must not be a copy-constructor based one,
- if the replicator type is a pointer, the provided replicator must not be `nullptr`,
- if the deleter type is a pointer, the provided deleter must not be `nullptr`,
- if the replicator type is a reference, the provided replicator must not be a temporal, and
- if the deleter type is a reference, the provided deleter must not be a temporal.

Note that, if the given pointer type is not compatible, the "master" constructor doesn't even get defined, if it is, the dummy `nullptr_t` parameter distinguishes it from all of the other constructors (this dummy parameter is _not_ optional, since making it so would lead to ambiguities).

Now, there are several constructor groups:

- default, copy, and move constructors,
- templated copy and move constructors,
- ownership taking constructors,
- `nullptr` constructors,
- `auto_ptr` constructors,
- `unique_ptr` constructors,
- `shared_ptr` constructors, and
- `weak_ptr` constructors.

We'll treat each in turn.

##### Default, Copy, and Move Constructors

The simplest of them all:

````c++
constexpr value_ptr() noexcept : value_ptr{pointer_type(), replicator_type(), deleter_type(), nullptr} {}
constexpr value_ptr(value_ptr const &other) noexcept : value_ptr{other.get_replicator()(other.get()), other.get_replicator(), other.get_deleter(), nullptr} {}
constexpr value_ptr(value_ptr &&other) noexcept : value_ptr{other.release(), std::move(other.get_replicator()), std::move(other.get_deleter()), nullptr} {}
````

They simply provide defaults and delegate to the "master" constructor.

##### Templated Copy and Move Constructors

These are similar to the ones above:

````c++
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(value_ptr<T2, TRep2, TDel2> const &other) noexcept : value_ptr{other.get_replicator()(other.get()), other.get_replicator(), other.get_deleter(), nullptr} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(value_ptr<T2, TRep2, TDel2> &&other) noexcept : value_ptr{other.release(), std::move(other.get_replicator()), std::move(other.get_deleter()), nullptr} {}
````

These allow the interoperability of compatible `value_ptr`s.

##### Ownership Taking Constructors

These are rather basic:

````c++
template <typename T2>
constexpr value_ptr(T2 *p) noexcept : value_ptr{p, replicator_type(), deleter_type(), nullptr} {}
template <typename T2, typename TRep2>
constexpr value_ptr(T2 *p, TRep2&& replicator) noexcept : value_ptr{p, std::forward<TRep2>(replicator), deleter_type(), nullptr} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(T2 *p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter), nullptr} {}
````

They simply acquire ownership of the given pointer (which must be compatible) and, optionally, set the replicator and deleter.

##### `nullptr` Constructors

Similar to the above ones are:

````c++
constexpr value_ptr(nullptr_t) noexcept : value_ptr{nullptr, replicator_type(), deleter_type(), nullptr} {}
template <typename TRep2>
constexpr value_ptr(nullptr_t, TRep2&& replicator) noexcept : value_ptr{nullptr, std::forward<TRep2>(replicator), deleter_type(), nullptr} {}
template <typename TRep2, typename TDel2>
constexpr value_ptr(nullptr_t, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{nullptr, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter), nullptr} {}
````

These are in place to allow initialization fron a `nullptr`.

##### `auto_ptr` Constructors

Although deprecated, we provide support for `auto_ptr`s.

The copy constructors are:

````c++
template <typename T2>
constexpr value_ptr(std::auto_ptr<T2> const &p) noexcept : value_ptr{replicator_type()(p.get())} {}
template <typename T2, typename TRep2>
constexpr value_ptr(std::auto_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator)} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::auto_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

And they simply delegate to the ownership taking ones after replicating .

The move constructors are:

````c++
template <typename T2>
constexpr value_ptr(std::auto_ptr<T2> &&p) noexcept : value_ptr{p.release()} {}
template <typename T2, typename TRep2>
constexpr value_ptr(std::auto_ptr<T2> &&p, TRep2&& replicator) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator)} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::auto_ptr<T2> &&p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

And they simply delegate to the ownership taking ones after `release`ing.

##### `unique_ptr` Constructors

Support for `unique_ptr`s is similar to the above, but a `unique_ptr` has an implicit deleter which we try to acquire.

The copy constructors are:

````c++
template <typename T2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p) noexcept : value_ptr{replicator_type()(p.get()), replicator_type(), std::forward<TDel2>(p.get_deleter())} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(p.get_deleter())} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

And the move constructors are:

````c++
template <typename T2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p) noexcept : value_ptr{p.release(), replicator_type(), std::forward<TDel2>(p.get_deleter())} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p, TRep2&& replicator) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(p.get_deleter())} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

##### `shared_ptr` Constructors

Support for `shared_ptr`s is only provided in the form of copy constructors:

````c++
template <typename T2>
constexpr value_ptr(std::shared_ptr<T2> const &p) noexcept : value_ptr{replicator_type()(p.get()), replicator_type(), std::get_deleter(p)} {}
template <typename T2, typename TRep2>
constexpr value_ptr(std::shared_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::get_deleter(p)} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::shared_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

This is because in order to exploit a moving `shared_ptr`, we should first ensure that it is a `unique` one, if not, we can only copy it, but even if it is, having `shared_ptr` release its hold on the managed pointer is folly.

##### `weak_ptr` Constructors

Similarly, support for `weak_ptr` is only provided in the form of copy constructors:

````c++
template <typename T2>
constexpr value_ptr(std::weak_ptr<T2> const &p) noexcept : value_ptr{p.lock()} {}
template <typename T2, typename TRep2>
constexpr value_ptr(std::weak_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{p.lock(), std::forward<TRep2>(replicator)} {}
template <typename T2, typename TRep2, typename TDel2>
constexpr value_ptr(std::weak_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.lock(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}
````

These simply `lock()` the `weak_ptr` into a temporary and delegate to the `shared_ptr` constructors above.

#### Assignments

There are several assignment groups:

- Copy and move assignment,
- `nullptr` assignment, and
- Templated copy and move assignment.

We treat each in turn.

##### Copy and Move Assignment

Copy and move assignments are pretty standard:

````c++
value_ptr &operator=(value_ptr other) { swap(other); return *this; }
value_ptr &operator=(value_ptr &&other) { swap(std::move(other)); return *this; }
````

They simply apply the [copy and swap idiom](http://stackoverflow.com/a/3279550).

##### `nullptr` Assignment

Assigning a `nullptr` is achieved by:

````c++
value_ptr &operator=(nullptr_t) noexcept { reset(); return *this; }
````

Trivial.

##### Templated Copy and Move Assignment

````c++
template <typename T2, typename TRep2, typename TDel2>
typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, TRep2, TDel2> other) { swap(other); return *this; }
template <typename T2, typename TRep2, typename TDel2>
typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, TRep2, TDel2> &&other) { swap(std::move(other)); return *this; }
````

#### Safe `bool` Conversion

The safe `bool` conversion operator is defined as:

````c++
constexpr operator __unspecified_bool_type() const noexcept { return nullptr == get() ? nullptr : &value_ptr::c; }
````

Where `__unspecified_bool_type` is as defined above.
Note that this is the semantic equivalent to converting a pointer to a `bool`, ie. returning `true` if not equal to `nullptr`.

#### Observers

The class' observers are closely derived from those of `unique_ptr`.

We'll treat each in turn.

##### `operator*`

This operator is defined as:

````c++
constexpr reference_type operator*() const { return *get(); }
````

and it merely returns the pointed-to element.

##### `operator->`

This operator is defined as:

````c++
constexpr pointer_type operator->() const noexcept { return get(); }
````

and it returns the pointer proper.

##### `get()`

This method is defined as:

````c++
constexpr pointer_type get() const noexcept { return std::get<0>(c); }
````

and simply gets the pointer proper.

##### `get_replicator()`

These methods are defined as:

````c++
replicator_reference get_replicator() noexcept { return std::get<1>(c); }
constexpr replicator_const_reference get_replicator() const noexcept { return std::get<1>(c); }
````

And they return a reference (`const` or otherwise) to the replicator being used.

##### `get_deleter()`

These methods are defined as:

````c++
deleter_reference get_deleter() noexcept { return std::get<2>(c); }
constexpr deleter_const_reference get_deleter() const noexcept { return std::get<2>(c); }
````

And they return a reference (`const` or otherwise) to the deleter being used.

#### Modifiers

Modifiers are, again, modelled after `unique_ptr`'s ones.

We'll treat each in turn.

##### `release`

This method is defined as:

````c++
pointer_type release() noexcept { pointer_type old = get(); std::get<0>(c) = nullptr; return old; }
````

Releasing ownership of the stored element and returning a pointer to it.

##### `reset`

This method is defined as:

````c++
void reset(pointer_type _p = pointer_type()) noexcept { if (_p != get()) { get_deleter()(get()); std::get<0>(c) = _p; } }
````

Resetting the managed pointer to the one given (`nullptr` by default).

##### Templated `swap`

This templated method is defined as:

````c++
template <typename T2, typename TRep2, typename TDel2>
typename enable_if_compatible<T2, void>::type swap(value_ptr<T2, TRep2, TDel2> &other) noexcept { using std::swap; swap(c, other.c); }
````

Which basically swaps compatible `value_ptr`s.

### Non Member Functions

A number of facilities are implemented as non-member functions.
These are treated next.

#### Swap Overloads

Overloads of the `swap` function are defined as:

````c++
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &x, value_ptr<T2, R2, D2> &y) noexcept { x.swap(y); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &&x, value_ptr<T2, R2, D2> &y) noexcept { y.swap(std::move(x)); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &x, value_ptr<T2, R2, D2> &&y) noexcept { x.swap(std::move(y)); }
````

Which simply call the class' swap method.

#### Equality Overloads

Pointer-equality is realized by:

````c++
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator==(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return x.get() == y.get(); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator!=(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return !(x == y); }
````

Comparing pointer _values_ rather than content.

#### `nullptr` Equality Overloads

The following are specializations of the overloads given above for the `nullptr_t` case:

````c++
template <class T, class R, class D>
inline bool operator==(value_ptr<T, R, D> const &x, nullptr_t y) { return x.get() == y; }
template <class T, class R, class D>
inline bool operator!=(value_ptr<T, R, D> const &x, nullptr_t y) { return !(x == y); }
template <class T, class R, class D>
inline bool operator==(nullptr_t x, value_ptr<T, R, D> const &y) { return x == y.get(); }
template <class T, class R, class D>
inline bool operator!=(nullptr_t x, value_ptr<T, R, D> const &y) { return !(x == y); }
````

Both comparing left and right.

#### Comparison Overloads

Pointer comparison is achieved by:

````c++
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator< (value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) {
  using CT = typename std::common_type<typename value_ptr<T1, R1, D1>::pointer_type, typename value_ptr<T2, R2, D2>::pointer_type>::type;
  return std::less<CT>()(x.get(), y.get());
}
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator> (value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return y < x; }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator<=(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return !(y < x); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator>=(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return !(x < y); }
````

Note the conversion to a common type in `operator<`.
