# value_ptr

A value semantics smart pointer for C++.

## Quick Start

In order to use this class you must simply do:

````c++
#include "value_ptr.h"
````

Now you can for example write:

````c++
value_ptr<std::string> string_vp = new std::string("Hello World!");
````

From this point on, `value_ptr` will care for the pointer given and will `delete` it when appropriate.

If you want to control the replication or deleting procedures to use, you can do something like:

````c++
auto someClass_vp = value_ptr<someClass>(nullptr, some_handler_object);
````

Using the `value_ptr` is similar to using a `unique_ptr`, even the same names are used.

* * *

## Index

- [Introduction](#introduction)
  - [Disclaimer](#disclaimer)
- [Features](#features)
  - [Answers to Questions Posed in Proposal N3339](#answers-to-questions-posed-in-proposal-n3339)
    - [Should `value_ptr` be specialized to work with array types à la unique_ptr?](#should-value_ptr-be-specialized-to-work-with-array-types-à-la-unique_ptr)
    - [Should `value_ptr` take an `allocator` argument in addition to a `replicator` and a `deleter`?](#should-value_ptr-take-an-allocator-argument-in-addition-to-a-replicator-and-a-deleter)
    - [This implementation assumes that the `replicator` and `deleter` types are stateless; are these viable assumptions? If not, what policies should apply when they are being copied during a `value_ptr` copy?](#this-implementation-assumes-that-the-replicator-and-deleter-types-are-stateless-are-these-viable-assumptions-if-not-what-policies-should-apply-when-they-are-being-copied-during-a-value_ptr-copy)
    - [With which, if any, standard smart pointers should this template innately interoperate, and to what degree?](#with-which-if-any-standard-smart-pointers-should-this-template-innately-interoperate-and-to-what-degree)
    - [What color should the bicycle shed be painted?](#what-color-should-the-bicycle-shed-be-painted)
- [Usage](#usage)
  - [Handlers](#handlers)

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
- safe-bool conversion,
- full (1-dimensional) array support.

### Answers to Questions Posed in Proposal N3339

In [proposal N3339](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf), a couple of questions were posed, here, we present our answers (so far) to them.

#### Should `value_ptr` be specialized to work with array types à la unique_ptr?

On the one hand, arrays are not _values_ themselves in ANSI C / ISO C++, but rather they decay into pointers, so, a priori, no, we should not.

On the other hand, this would be extremely useful.

The only drawback is that in order to properly clone an array, we must somehow know its size given just a pointer to it, and this can only be done in an ABI dependent manner, and, even then, not for every possible type.

That being said, full support for 1-dimensional arrays has been added, and the particular restrictions that may apply depend on the underlying ABI being used.
Only one ABI is implemented this far: the [Itanium C++ ABI](https://mentorembedded.github.io/cxx-abi/abi.html), and its restrictions are:

- if the underlying type `T` has a [trivial destructor](http://en.cppreference.com/w/cpp/language/destructor#Trivial_destructor), then you may _not_ use it thus: `value_ptr<T[]>`, but must instead do: `value_ptr<T[n]>` for some compile-time constant `n`.
  This is because in order for the size to be determined, an _array cookie_ must be present, and for that to happenm, the Itanium ABI demands the constructor _not_ be trivial.

In the future, we may provide additional ABIs as we find the time (and sources).

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

## Usage

The `value_ptr` template can be used pretty much like a `unique_ptr` can.
It supports the methods `reset`, `release`, `get`, `operator*`, and `operator->` with the same semantics as `unique_ptr` exhibits, and it can be safely cast to `bool`.
Furthermore, if the template type parameter is an array type, it supports both overloads (`const` and non-`const`) of `operator[]`.

It additionally supports the `get_replicator` and `get_deleter` methods to obtain or modify the underlying replicator and deleter objects.

### Handlers

Deleters are objects having `destroy` and `replicate` methods, and a static `bool slice_safe` member.

The `destroy` method takes a pointer to a constant object of type `T` and takes care of its proper destruction and memory reclamation.

The `replicate` method takes a pointer to a constant object of type `T` and returns a pointer to a replica of it.

The `slice_safe` member should indicate whether the other methods may be passed a pointer to a base class and behave polymorphically.

You can provide your own handler if you so choose, but sane defaults are already provided by the `default_handler` class.

Do note, however, that a handler intended to work with arrays will necessarily depend on an ABI definition.
