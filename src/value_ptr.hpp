#ifndef VALUE_PTR__
#define VALUE_PTR__

#include <type_traits>
#include <functional>
#include <exception>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>
#include <new>


/**
 * Static class to encapsulate ABI dependent operations
 *
 */
class Abi {
  public:
    /**
     * Return the size of the array cookie needed
     *
     * @param T  Underlying type of the array
     * @return the size of the array cookie needed
     */
    template <typename T> static constexpr std::size_t arrayCookieLen() noexcept;

    /**
     * Return the size of the pointed-to array
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     * @return the size of the pointed-to array
     * @throws abi_error  In case the size cannot be determined
     */
    template <typename T> static std::size_t arraySize(T const *p) noexcept;

    /**
     * Return a new array, including cookie if needed, but do NOT call constructors
     *
     * @param T  Underlying type of the array
     * @param n  Number of elements in the allocated array
     * @return a pointer to the allocated array
     * @throws std::bad_alloc  In case the underlying operation throws
     */
    template <typename T> static T *newArray(std::size_t n);

    /**
     * Delete an array created by newArray<T>, including cookie if needed, but do NOT call destructors
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     */
    template <typename T> static void delArray(T const *p) noexcept;
};

/**
 * Static class to encapsulate Itanium ABI operations
 *
 */
class Itanium : public Abi {
  public:
    /**
     * Return the size of the array cookie needed
     *
     * @param T  Underlying type of the array
     * @return the size of the array cookie needed
     */
    template <typename T>
    static constexpr std::size_t arrayCookieLen() noexcept override {
      return __has_trivial_destructor(T) ? 0 : std::max(sizeof(std::size_t), alignof(T));
    }

    /**
     * Return the size of the pointed-to array
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     * @return the size of the pointed-to array
     * @throws abi_error  In case the size cannot be determined
     */
    template <typename T>
    static std::size_t arraySize(T const *p) noexcept override {
      static_assert(!__has_trivial_destructor(T), "array type has trivial destructor");

      return reinterpret_cast<std::size_t const *>(p)[-1];
    }

    /**
     * Return a new array, including cookie if needed, but do NOT call constructors
     *
     * @param T  Underlying type of the array
     * @param n  Number of elements in the allocated array
     * @return a pointer to the allocated array
     * @throws std::bad_alloc  In case the underlying operation throws
     */
    template <typename T>
    static T *newArray(std::size_t n) override {
      std::size_t padding = arrayCookieLen<T>();
      T *ret = reinterpret_cast<T *>(new char[n * sizeof(T) + padding] + padding);

      if (padding) {
        reinterpret_cast<std::size_t *>(ret)[-1] = n;
      }

      return ret;
    }

    /**
     * Delete an array created by newArray<T>, including cookie if needed, but do NOT call destructors
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     */
    template <typename T>
    static void delArray(T const *p) noexcept override {
      delete[] (reinterpret_cast<char const *>(p) - arrayCookieLen<T>());
    }
};

/**
 * Specialization of conditional that mirrors its bool value to a type
 *
 * @param C  Condition to "lift" to a type
 */
template <bool C>
using condition = std::conditional<C, std::true_type, std::false_type>;

/**
 * Metaprogramming class to detect the presence of a (possibly inherited) "placement clone" method (possibly utilizing covariant return types)
 *
 * A "placement clone" method is a (virtual) method of a (polymorphic) class
 * that takes a single "void *" parameter, is itself const, and returns a
 * pointer to (a base class of) T.
 *
 * Adapted from: http://stackoverflow.com/a/10707822
 *
 * @param T  Class to check for
 * @var bool value  True if T has a placement clone method, false otherwise
 */
template <typename T>
struct is_placement_cloneable {
  protected:
    /**
     * Default case
     *
     * Always succeeds (note the "..." in the argument specification), resolves
     * to false (std::false_type).
     *
     * @param <unnamed>  Ignored
     */
    template <typename>
    static constexpr auto test(...) -> std::false_type;

    /**
     * Test for the existence of the "clone" method
     *
     * This metamethod will only be defined when decltype(&S::clone) succeeds,
     * which will only happen when S has a method named "clone" itself.
     *
     * It will also try to resolve "decltype(test(&S::clone, nullptr))", since
     * this appears to be its return type; this in turn triggers the signature
     * recognition metamethod below.
     *
     * Note that this method will have the same return type as the metamethod
     * below, if substitution succeeds, or the same as the one above, if
     * substitution fails.
     *
     * @param S  Base class under which to look for a "clone" method
     */
    template <typename S>
    static constexpr auto test(decltype(&S::clone))
      -> decltype(test(&S::clone, nullptr));

    /**
     * Test for the correct signature for "clone"
     *
     * This metamethod will check that the "clone" method identified above has
     * the required signature. Note the R parameter: this is added in order to
     * allow for covariant return types in the resolved method (this is later
     * checked for in the metamethod's return type, along with a check for size
     * equality: this prevents us from falsely treating an inherited "clone"
     * method as valid if the class that defines it has a different size than
     * the one we're looking for). The parameter pack U is added in order to
     * detect "clone" methods accepting optional arguments, this is later
     * checked in the condition.
     *
     * @param S  Base class under which to look for a "clone" method
     * @param R  Type of object the "clone" method returns a pointer to
     */
    template <typename S, typename R, typename ...U>
    static constexpr auto test(R *(S::*)(void *, U...) const, nullptr_t)
      -> typename condition<
        sizeof(R) == sizeof(S) &&
        std::is_base_of<R, S>::value &&
        std::is_same<R *, decltype(std::declval<S>().clone(nullptr))>::value
      >::type;

  public:
    /**
     * A class will be placement cloneable if it is a polymorphic one and it
     * contains a suitably defined "clone" method.
     *
     */
    static constexpr bool value = std::is_polymorphic<T>::value && decltype(test<T>(nullptr))::value;
};

/**
 * Specialization of is_placement_cloneable for array types
 *
 * An array type is never considered placement clonable, since it cannot
 * itself have a "placement clone" method.
 *
 */
template <typename T>
struct is_placement_cloneable<T[]> {
  static constexpr bool value = false;
};

/**
 * Metaprogramming class to detect the presence of a (possibly inherited) "clone" method (possibly utilizing covariant return types)
 *
 * A "clone" method is a (virtual) method of a (polymorphic) class that takes
 * no parameters, is itself const, and returns a pointer to (a base class of)
 * T.
 *
 * Adapted from: http://stackoverflow.com/a/10707822
 *
 * @param T  Class to check for
 * @var bool value  True if T has a clone method, false otherwise
 */
template <typename T>
struct is_cloneable {
  protected:
    /**
     * Default case
     *
     * Always succeeds (note the "..." in the argument specification), resolves
     * to false (std::false_type).
     *
     * @param <unnamed>  Ignored
     */
    template <typename>
    static constexpr auto test(...) -> std::false_type;

    /**
     * Test for the existence of the "clone" method
     *
     * This metamethod will only be defined when decltype(&S::clone) succeeds,
     * which will only happen when S has a method named "clone" itself.
     *
     * It will also try to resolve "decltype(test(&S::clone, nullptr))", since
     * this appears to be its return type; this in turn triggers the signature
     * recognition metamethod below.
     *
     * Note that this method will have the same return type as the metamethod
     * below, if substitution succeeds, or the same as the one above, if
     * substitution fails.
     *
     * @param S  Base class under which to look for a "clone" method
     */
    template <typename S>
    static constexpr auto test(decltype(&S::clone))
      -> decltype(test(&S::clone, nullptr));

    /**
     * Test for the correct signature for "clone"
     *
     * This metamethod will check that the "clone" method identified above has
     * the required signature. Note the R parameter: this is added in order to
     * allow for covariant return types in the resolved method (this is later
     * checked for in the metamethod's return type, along with a check for size
     * equality: this prevents us from falsely treating an inherited "clone"
     * method as valid if the class that defines it has a different size than
     * the one we're looking for). The parameter pack U is added in order to
     * detect "clone" methods accepting optional arguments, this is later
     * checked in the condition.
     *
     * @param S  Base class under which to look for a "clone" method
     * @param R  Type of object the "clone" method returns a pointer to
     */
    template <typename S, typename R, typename ...U>
    static constexpr auto test(R *(S::*)(U...) const, nullptr_t)
      -> typename condition<
        sizeof(R) == sizeof(S) &&
        std::is_base_of<R, S>::value &&
        std::is_same<R *, decltype(std::declval<S>().clone())>::value
      >::type;

  public:
    /**
     * A class will be cloneable if it is a polymorphic one and it contains a
     * suitably defined "clone" method.
     *
     */
    static constexpr bool value = std::is_polymorphic<T>::value && decltype(test<T>(nullptr))::value;
};

/**
 * Specialization of is_clonable for array types
 *
 * An array type is considered cloneable itself if its constituent type is
 * placement cloneable.
 *
 */
template <typename T>
struct is_cloneable<T[]> {
  static constexpr bool value = is_placement_cloneable<T>::value;
};

/**
 * Metaprogramming class to provide a default replicator using the class' copy constructor
 *
 * @param T  Class to provide a replicator for
 * @param ABI  ABI adapter class to use (Itanium by default)
 */
template <typename T, typename ABI = Itanium>
struct default_copy {
  /**
   * Refuse to accept types which we do not know how to copy
   *
   * Since this replicator implicitly uses the underlying type's copy
   * constructor, we can't do without that.
   *
   */
  static_assert(std::is_copy_constructible<T>::value, "default_copy requires a copy constructor");

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U> const &) noexcept {}
  template <typename U> default_copy(default_copy<U> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U> const &) noexcept { return *this; }
  template <typename U> default_copy &operator=(default_copy<U> &&) noexcept { return *this; }
  virtual ~default_copy() noexcept {};

  /**
   * Replication implementation
   *
   * The "operator()" in this class returns a new object of the underlying
   * class by calling its copy constructor on the given object, it returns
   * nullptr if a nullptr is given.
   *
   * @param p  Pointer to the object to copy
   * @return either nullptr if nullptr is given, or a new object copied from p
   */
  T *operator()(T const *p) const { return nullptr != p ? new T{*p} : nullptr; }
};

/**
 * Specialization of default_copy for array types
 *
 */
template <typename T, typename ABI>
struct default_copy<T[], ABI> {
  /**
   * Refuse to accept types which we do not know how to copy
   *
   * Since this replicator implicitly uses the underlying type's copy
   * constructor, we can't do without that.
   *
   */
  static_assert(std::is_copy_constructible<T>::value, "default_copy requires a copy constructor");

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U> const &) noexcept {}
  template <typename U> default_copy(default_copy<U> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U> const &) noexcept { return *this; }
  template <typename U> default_copy &operator=(default_copy<U> &&) noexcept { return *this; }
  virtual ~default_copy() noexcept {};

  /**
   * Replication implementation
   *
   * The "operator()" in this class returns a new array of objects of the
   * underlying class by performing a placement new using its copy constructor
   * on each given object, it returns nullptr if a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array copied from p
   */
  T *operator()(T const *p) const {
    if (nullptr == p) {
      return nullptr;
    }

    std::size_t i, n = ABI::template arraySize<T>(p);
    T *ret = ABI::template newArray<T>(n);

    try {
      for (i = 0; i < n; i++) {
        new(ret + i) T{p[i]};
      }
    } catch (...) {
      while (i--) {
        try { (ret + i)->~T(); } catch (...) { std::terminate(); }
      }
      ABI::template delArray<T>(ret);
      throw;
    }

    return ret;
  }
};

/**
 * Metaprogramming class to provide a default replicator using the class' "clone" method
 *
 * @param T  Class to provide a replicator for
 * @param ABI  ABI adapter class to use (Itanium by default)
 */
template <typename T, typename ABI = Itanium>
struct default_clone {
  /**
   * Refuse to accept types which we do not know how to clone
   *
   * Since this replicator implicitly uses the underlying type's "clone"
   * method, we can't do without that.
   *
   */
  static_assert(is_cloneable<T>::value, "default_clone requires a cloneable type");

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U> const &) noexcept {}
  template <typename U> default_clone(default_clone<U> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U> const &) noexcept { return *this; }
  template <typename U> default_clone &operator=(default_clone<U> &&) noexcept { return *this; }
  virtual ~default_clone() noexcept {};

  /**
   * Replication implementation
   *
   * The "operator()" in this class returns a new object of the underlying
   * class by calling its "clone" method on the given object, it returns
   * nullptr if a nullptr is given.
   *
   * @param p  Pointer to the object to copy
   * @return either nullptr if nullptr is given, or a new object cloned from p
   */
  T *operator()(T const *p) const { return nullptr != p ? p->clone() : nullptr; }
};

/**
 * Specialization of default_clone for array types
 *
 */
template <typename T, typename ABI>
struct default_clone<T[], ABI> {
  /**
   * Refuse to accept types which we do not know how to placement clone
   *
   * Since this replicator implicitly uses the underlying type's
   * "placement clone" method, we can't do without that.
   *
   */
  static_assert(is_placement_cloneable<T>::value, "default_clone requires a placement-cloneable type");

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U> const &) noexcept {}
  template <typename U> default_clone(default_clone<U> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U> const &) noexcept { return *this; }
  template <typename U> default_clone &operator=(default_clone<U> &&) noexcept { return *this; }
  virtual ~default_clone() noexcept {};

  /**
   * Replication implementation
   *
   * The "operator()" in this class returns a new array of objects of the
   * underlying class by performing a placement clone on each given object, it
   * returns nullptr if a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array cloned from p
   */
  T *operator()(T const *p) const {
    if (nullptr == p) {
      return nullptr;
    }

    std::size_t i, n = ABI::template arraySize<T>(p);
    T *ret = ABI::template newArray<T>(n);

    try {
      for (i = 0; i < n; i++) {
        (p + i)->clone(ret + i);
      }
    } catch (...) {
      while (i--) {
        try { (ret + i)->~T(); } catch (...) { std::terminate(); }
      }
      ABI::template delArray<T>(ret);
      throw;
    }

    return ret;
  }
};

/**
 * Metaprogramming class to automatically select the replication method to use
 *
 * @param T  Class to select a replication method for
 * @param use_clone  Whether a cloner should be used instead of a copier (this is filled-in by default)
 */
template <typename T, bool use_clone = is_cloneable<T>::value>
struct default_replicate;

/**
 * Partial specialization of the above template for cloning
 *
 * @param T  Class to provide a replicator for
 */
template <typename T>
struct default_replicate<T, true> : public default_clone<T> {
  /**
   * Replicators are implemented as "operator()"
   *
   * This class inherits from a default cloner and thus can be simply used
   * here.
   *
   */
  using default_clone<T>::operator();

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_replicate() noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> &&) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> const &) noexcept { return *this; }
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> &&) noexcept { return *this; }
  virtual ~default_replicate() noexcept {};
};

/**
 * Partial specialization of the above template for copying
 *
 * @param T  Class to provide a replicator for
 */
template <typename T>
struct default_replicate<T, false> : public default_copy<T> {
  /**
   * Replicators are implemented as "operator()"
   *
   * This class inherits from a default copier and thus can be simply used
   * here.
   *
   */
  using default_copy<T>::operator();

  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_replicate() noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> const &) noexcept {}
  template <typename U, bool V> default_replicate(default_replicate<U, V> &&) noexcept {}
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> const &) noexcept { return *this; }
  template <typename U, bool V> default_replicate &operator=(default_replicate<U, V> &&) noexcept { return *this; }
  virtual ~default_replicate() noexcept {};
};

/**
 * Metaprogramming class to automatically select the destruction method to use
 *
 * @param T  Class to select a destruction method for
 * @param ABI  ABI adapter class to use (Itanium by default)
 */
template <typename T, typename ABI = Itanium>
struct default_destroy {
  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_destroy() noexcept {}
  template <typename U, typename V> default_destroy(default_destroy<U, V> const &) noexcept {}
  template <typename U, typename V> default_destroy(default_destroy<U, V> &&) noexcept {}
  template <typename U, typename V> default_destroy &operator=(default_destroy<U, V> const &) noexcept { return *this; }
  template <typename U, typename V> default_destroy &operator=(default_destroy<U, V> &&) noexcept { return *this; }
  virtual ~default_destroy() noexcept {};

  /**
   * Destroyer implementation
   *
   * The "operator()" in this class simply applies delete to the given pointer.
   *
   * @param p  Pointer to the object to delete
   */
  void operator()(T const *p) const {
    static_assert(sizeof(T) > 0, "default_destroy cannot work on incomplete types");

    delete p;
  }
};

/**
 * Specialization of default_destroy for array types
 *
 */
template <typename T, typename ABI>
struct default_destroy<T[], ABI> {
  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   */
  default_destroy() noexcept {}
  template <typename U, typename V> default_destroy(default_destroy<U, V> const &) noexcept {}
  template <typename U, typename V> default_destroy(default_destroy<U, V> &&) noexcept {}
  template <typename U, typename V> default_destroy &operator=(default_destroy<U, V> const &) noexcept { return *this; }
  template <typename U, typename V> default_destroy &operator=(default_destroy<U, V> &&) noexcept { return *this; }
  virtual ~default_destroy() noexcept {};

  /**
   * Destroyer implementation
   *
   * The "operator()" in this class calls each object's destructor and then
   * deletes the substrate array.
   *
   * @param p  Pointer to the array to delete
   */
  void operator()(T const *p) const {
    static_assert(sizeof(T) > 0, "default_destroy cannot work on incomplete types");

    if (nullptr == p) {
      return;
    }

    std::size_t n = ABI::template arraySize<T>(p), i = n;

    try {
      while (i--) {
        (p + i)->~T();
      }
      ABI::template delArray<T>(p);
    } catch (...) {
      while (i--) {
        try { (p + i)->~T(); } catch (...) { std::terminate(); }
      }
      ABI::template delArray<T>(p);
      throw;
    }
  }
};

/**
 * Smart pointer with value-like semantics
 *
 * @param T  Underlying type to wrap
 * @param TRep  Replicator type to use
 * @param TDel  Deleter type to use
 */
template <typename T, typename TRep = default_replicate<T>, typename TDel = default_destroy<T>>
class value_ptr {
  public:
    /**
     * Export basic type alias for the underlying type
     *
     */
    using element_type          = typename std::remove_extent<T>::type;
    using pointer_type          = element_type *;
    using reference_type        = element_type &;
    using lvalue_reference_type = element_type &&;

    /**
     * Export basic type alias for the replicator type
     *
     */
    using replicator_type            = TRep;
    using replicator_reference       = typename std::add_lvalue_reference<replicator_type>::type;
    using replicator_const_reference = typename std::add_lvalue_reference<typename std::add_const<replicator_type>::type>::type;

    /**
     * Export basic type alias for the deleter type
     *
     */
    using deleter_type            = TDel;
    using deleter_reference       = typename std::add_lvalue_reference<deleter_type>::type;
    using deleter_const_reference = typename std::add_lvalue_reference<typename std::add_const<deleter_type>::type>::type;

  protected:
    /**
     * The internal state will consist of a tuple of:
     *  - a pointer to the underlying type
     *  - a replicator
     *  - a deleter
     *
     */
    using tuple_type = std::tuple<pointer_type, replicator_type, deleter_type>;

    /**
     * This is just a trick to provide safe bool conversion
     *
     */
    using __unspecified_bool_type = tuple_type value_ptr::*;

    /**
     * Convenience alias used to enable only if the underlying pointers would be compatible
     *
     * In case we're serving an array type, consider its underlying type
     * instead.
     */
    template <typename U, typename V = nullptr_t>
    using enable_if_compatible = std::enable_if<
      std::is_convertible<
        typename std::add_pointer<
          typename std::conditional<
            std::rank<T>::value == 1 && std::extent<T>::value == 0,
            typename std::remove_extent<U>::type,
            U
          >::type
        >::type,
        pointer_type
      >::value, V>;

    /**
     * Convenience alias used to enable only if we're serving an array type.
     *
     * An array type is take to be a one dimensional open array.
     *
     */
    template <typename U, typename V = nullptr_t>
    using enable_if_array = std::enable_if<std::rank<U>::value == 1 && std::extent<U>::value == 0, V>;

  public:
    /**
     * Default constructor
     *
     * Initializes to nullptr delegating to the "master" constructor below.
     *
     */
    constexpr value_ptr() noexcept : value_ptr{pointer_type(), replicator_type(), deleter_type(), nullptr} {}
    /**
     * Copy constructor
     *
     * Initializes using the to-be-copied replicator, delegating to the
     * "master" constructor below.
     *
     * @param other  Object to copy
     */
    constexpr value_ptr(value_ptr const &other) noexcept : value_ptr{other.get_replicator()(other.get()), other.get_replicator(), other.get_deleter(), nullptr} {}
    /**
     * Move constructor
     *
     * Initializes using the release()'d pointer, delegating to the
     * "master" constructor below.
     *
     * @param other  Object to move
     */
    constexpr value_ptr(value_ptr &&other) noexcept : value_ptr{other.release(), std::move(other.get_replicator()), std::move(other.get_deleter()), nullptr} {}

    /**
     * Templated copy constructor
     *
     * This copy constructor will accept an object derived from any instance of
     * the template parameters as long as the pointer types are compatible and
     * there exists an implicit conversion for the replicator and deleter
     * types; in all other respects it behaves like the above copy constructor.
     *
     * It delegates construction to the "master constructor" below.
     *
     * @param other  Object to copy
     */
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(value_ptr<T2, TRep2, TDel2> const &other) noexcept : value_ptr{other.get_replicator()(other.get()), other.get_replicator(), other.get_deleter(), nullptr} {}
    /**
     * Templated move constructor
     *
     * This move constructor will accept an object derived from any instance of
     * the template parameters as long as the pointer types are compatible and
     * there exists an implicit conversion for the replicator and deleter
     * types; in all other respects it behaves like the above move constructor.
     *
     * It delegates construction to the "master constructor" below.
     *
     * @param other  Object to move
     */
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(value_ptr<T2, TRep2, TDel2> &&other) noexcept : value_ptr{other.release(), std::move(other.get_replicator()), std::move(other.get_deleter()), nullptr} {}

    /**
     * Ownership taking initializing constructors
     *
     * These constructors take a compatible pointer and, optionally, a
     * replicator and a deleter and assume ownership of the pointer in
     * question and initialize the replicator and deleter.
     *
     * Note the use of "universal references" and perfect forwarding.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Pointer to take ownership of
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use
     */
    template <typename T2>
    constexpr value_ptr(T2 *p) noexcept : value_ptr{p, replicator_type(), deleter_type(), nullptr} {}
    template <typename T2, typename TRep2>
    constexpr value_ptr(T2 *p, TRep2&& replicator) noexcept : value_ptr{p, std::forward<TRep2>(replicator), deleter_type(), nullptr} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(T2 *p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter), nullptr} {}

    /**
     * Nullptr constructors
     *
     * These constructors work as the ones above, but they're specialized for
     * the nullptr constant.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param <unnamed>  Nullptr constant
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use
     */
    constexpr value_ptr(nullptr_t) noexcept : value_ptr{nullptr, replicator_type(), deleter_type(), nullptr} {}
    template <typename TRep2>
    constexpr value_ptr(nullptr_t, TRep2&& replicator) noexcept : value_ptr{nullptr, std::forward<TRep2>(replicator), deleter_type(), nullptr} {}
    template <typename TRep2, typename TDel2>
    constexpr value_ptr(nullptr_t, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{nullptr, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter), nullptr} {}

    /**
     * Auto_ptr converting copy-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a replication of an auto_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Auto_ptr to use as replication origin
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use
     */
    template <typename T2>
    constexpr value_ptr(std::auto_ptr<T2> const &p) noexcept : value_ptr{replicator_type()(p.get())} {}
    template <typename T2, typename TRep2>
    constexpr value_ptr(std::auto_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator)} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::auto_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Auto_ptr converting move-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from an auto_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Auto_ptr to use as pointer origin
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use
     */
    template <typename T2>
    constexpr value_ptr(std::auto_ptr<T2> &&p) noexcept : value_ptr{p.release()} {}
    template <typename T2, typename TRep2>
    constexpr value_ptr(std::auto_ptr<T2> &&p, TRep2&& replicator) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator)} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::auto_ptr<T2> &&p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Unique_ptr converting copy-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a replication of a unique_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Unique_ptr to use as replication origin
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use (when not provided, use unique_ptr's one)
     */
    template <typename T2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p) noexcept : value_ptr{replicator_type()(p.get()), replicator_type(), std::forward<TDel2>(p.get_deleter())} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(p.get_deleter())} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Unique_ptr converting move-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a unique_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Unique_ptr to use as pointer origin
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use (when not provided, use unique_ptr's one)
     */
    template <typename T2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p) noexcept : value_ptr{p.release(), replicator_type(), std::forward<TDel2>(p.get_deleter())} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p, TRep2&& replicator) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(p.get_deleter())} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::unique_ptr<T2, TDel2> &&p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.release(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Shared_ptr converting copy-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a replication of a shared_ptr.
     *
     * Note that there's no point in providing a move-constructor here since
     * there's no way to release the pointer being held by a shared_ptr and
     * we'd need to copy the pointed-to object anyway.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Shared_ptr to use as replication origin
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use (when not provided, use unique_ptr's one)
     */
    template <typename T2>
    constexpr value_ptr(std::shared_ptr<T2> const &p) noexcept : value_ptr{replicator_type()(p.get()), replicator_type(), std::get_deleter(p)} {}
    template <typename T2, typename TRep2>
    constexpr value_ptr(std::shared_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::get_deleter(p)} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::shared_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{replicator(p.get()), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Weak_ptr converting copy-constructors
     *
     * These constructors simply lock a weak_ptr and delegate to the ones given
     * above.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Weak_ptr to use
     * @param replicator  Replicator to use
     * @param deleter  Deleter to use (when not provided, use unique_ptr's one)
     */
    template <typename T2>
    constexpr value_ptr(std::weak_ptr<T2> const &p) noexcept : value_ptr{p.lock()} {}
    template <typename T2, typename TRep2>
    constexpr value_ptr(std::weak_ptr<T2> const &p, TRep2&& replicator) noexcept : value_ptr{p.lock(), std::forward<TRep2>(replicator)} {}
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(std::weak_ptr<T2> const &p, TRep2&& replicator, TDel2&& deleter) noexcept : value_ptr{p.lock(), std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {}

    /**
     * Nullptr assignment operator
     *
     * This assignment operator specializes the nullptr_t case by simply
     * resetting.
     *
     * @param <unnamed>  Nullptr to assign
     * @return the assigned object
     */
    value_ptr &operator=(nullptr_t) noexcept { reset(); return *this; }

    /**
     * Copy-assignment operator
     *
     * This copy-assignment operator implements the "copy and swap" idiom.
     *
     * @param other  Object to copy-assign
     * @return the assigned object
     */
    value_ptr &operator=(value_ptr other) { swap(other); return *this; }

    /**
     * Move-assignment operator
     *
     * This Move-assignment operator implements the "move and swap" idiom.
     *
     * @param other  Object to move-assign
     * @return the assigned object
     */
    value_ptr &operator=(value_ptr &&other) { swap(std::move(other)); return *this; }

    /**
     * Templated copy-assignment operator
     *
     * This copy-assignment operator accepts any compatible object, it
     * implements the "copy and swap" idiom.
     *
     * @param other  Object to copy-assign
     * @return the assigned object
     */
    template <typename T2, typename TRep2, typename TDel2>
    typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, TRep2, TDel2> other) { swap(other); return *this; }

    /**
     * Templated move-assignment operator
     *
     * This move-assignment operator accepts any compatible object, it
     * implements the "move and swap" idiom.
     *
     * @param other  Object to move-assign
     * @return the assigned object
     */
    template <typename T2, typename TRep2, typename TDel2>
    typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, TRep2, TDel2> &&other) { swap(std::move(other)); return *this; }

    /**
     * Safe bool conversion operator
     *
     * This operator implements a safe bool conversion.
     *
     */
    constexpr operator __unspecified_bool_type() const noexcept { return nullptr == get() ? nullptr : &value_ptr::c; }

    /**
     * Virtual destructor
     *
     * The destructor merely resets the object.
     *
     */
    virtual ~value_ptr() noexcept { reset(); }

    /**
     * Const-reference operator[]
     *
     * Trying to access past the array's bounds is considered undefined
     * behavior.
     *
     * Note the "template <typename U = T>" trick used: this allows us to
     * use type traits on the main template type (based on:
     * http://stackoverflow.com/a/21464113); this effectively enables the
     * operator as a whole only when serving an array type.
     *
     * @param i  Index to retrieve
     * @return a reference to the i-th entry in the array
     */
    template <typename U = T>
    constexpr typename enable_if_array<U, lvalue_reference_type>::type operator[](std::size_t i) const { return get()[i]; }

    /**
     * Non-const reference operator[]
     *
     * Trying to access past the array's bounds is considered undefined
     * behavior.
     *
     * Note the "template <typename U = T>" trick used: this allows us to
     * use type traits on the main template type (based on:
     * http://stackoverflow.com/a/21464113); this effectively enables the
     * operator as a whole only when serving an array type.
     *
     * @param i  Index to retrieve
     * @return a reference to the i-th entry in the array
     */
    template <typename U = T>
    typename enable_if_array<U, reference_type>::type operator[](std::size_t i) { return get()[i]; }

    /**
     * Get the pointed-to object
     *
     * @return the pointed-to object as a reference
     */
    constexpr reference_type operator*() const { return *get(); }

    /**
     * Get the current pointer
     *
     * @return the pointer being held
     */
    constexpr pointer_type operator->() const noexcept { return get(); }

    /**
     * Return the pointer part of the internal state
     *
     * @return the current pointer
     */
    constexpr pointer_type get() const noexcept { return std::get<0>(c); }

    /**
     * Get a modifiable reference to the current replicator
     *
     * @return a reference to the current replicator
     */
    replicator_reference get_replicator() noexcept { return std::get<1>(c); }

    /**
     * Get an unmodifiable reference to the current replicator
     *
     * @return a const reference to the current replicator
     */
    constexpr replicator_const_reference get_replicator() const noexcept { return std::get<1>(c); }

    /**
     * Get a modifiable reference to the current deleter
     *
     * @return a reference to the current deleter
     */
    deleter_reference get_deleter() noexcept { return std::get<2>(c); }

    /**
     * Get an unmodifiable reference to the current deleter
     *
     * @return a const reference to the current deleter
     */
    constexpr deleter_const_reference get_deleter() const noexcept { return std::get<2>(c); }

    /**
     * Release ownership of the current pointer and reset it to nullptr
     *
     * @return the previously owned pointer
     */
    pointer_type release() noexcept { pointer_type old = get(); std::get<0>(c) = nullptr; return old; }

    /**
     * Reset the internal pointer to the given value (nullptr, by default)
     *
     * @param p  New value to acquire
     */
    void reset(pointer_type p = pointer_type()) noexcept { if (p != get()) { get_deleter()(get()); std::get<0>(c) = p; } }

    /**
     * Swap the internal state with a compatible value_ptr
     *
     * @param other  The value_ptr to swap values with
     */
    template <typename T2, typename TRep2, typename TDel2>
    typename enable_if_compatible<T2, void>::type swap(value_ptr<T2, TRep2, TDel2> &other) noexcept { using std::swap; swap(c, other.c); }

    /**
     * Swap the internal state with a compatible value_ptr (rvalue overload)
     *
     * @param other  The value_ptr to swap values with
     */
    template <typename T2, typename TRep2, typename TDel2>
    typename enable_if_compatible<T2, void>::type swap(value_ptr<T2, TRep2, TDel2> &&other) noexcept { using std::swap; swap(c, other.c); }

  protected:
    /**
     * Construct a new value_ptr with the given arguments and perform sanity checks
     *
     * The sanity checks performed are:
     * - if the pointed-to type is polymorphic, the replicator cannot be a default_copy one,
     * - if the replicator type is a pointer, it cannot be initialized with nullptr,
     * - if the deleter type is a pointer, it cannot be initialized with nullptr,
     * - if the replicator type is a reference, it cannot be initialized with a temporary,
     * - if the deleter type is a reference, it cannot be initialized with a temporary.
     *
     *
     * @param p  Pointer to take ownership of
     * @param replicator  Replicator object to use
     * @param deleter  Deleter object to use
     * @param <unnamed>  nullptr_t parameter to use for disambiguation
     */
    template <typename T2, typename TRep2, typename TDel2>
    constexpr value_ptr(T2 *p, TRep2&& replicator, TDel2&& deleter, typename enable_if_compatible<T2>::type) noexcept : c{p, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {
      static_assert(!std::is_polymorphic<T>::value || !std::is_same<TRep, default_replicate<T, false>>::value, "would slice when copying");
      static_assert(!std::is_pointer<replicator_type>::value || !std::is_same<TRep2, nullptr_t>::value, "constructed with null function pointer replicator");
      static_assert(!std::is_pointer<deleter_type>::value || !std::is_same<TDel2, nullptr_t>::value, "constructed with null function pointer deleter");
      static_assert(!std::is_reference<replicator_type>::value || !std::is_rvalue_reference<TRep2>::value, "rvalue replicator bound to reference");
      static_assert(!std::is_reference<deleter_type>::value || !std::is_rvalue_reference<TDel2>::value, "rvalue replicator bound to reference");
    }

    /**
     * Construct a new value_ptr from a nullptr
     *
     * The sanity checks performed are:
     * - if the pointed-to type is polymorphic, the replicator cannot be a default_copy one,
     * - if the replicator type is a pointer, it cannot be initialized with nullptr,
     * - if the deleter type is a pointer, it cannot be initialized with nullptr,
     * - if the replicator type is a reference, it cannot be initialized with a temporary,
     * - if the deleter type is a reference, it cannot be initialized with a temporary.
     *
     *
     * @param <unnamed>  Nullptr to use
     * @param replicator  Replicator object to use
     * @param deleter  Deleter object to use
     * @param <unnamed>  nullptr_t parameter to use for disambiguation
     */
    template <typename TRep2, typename TDel2>
    constexpr value_ptr(nullptr_t, TRep2&& replicator, TDel2&& deleter, nullptr_t) noexcept : c{nullptr, std::forward<TRep2>(replicator), std::forward<TDel2>(deleter)} {
      static_assert(!std::is_polymorphic<T>::value || !std::is_same<TRep, default_replicate<T, false>>::value, "would slice when copying");
      static_assert(!std::is_pointer<replicator_type>::value || !std::is_same<TRep2, nullptr_t>::value, "constructed with null function pointer replicator");
      static_assert(!std::is_pointer<deleter_type>::value || !std::is_same<TDel2, nullptr_t>::value, "constructed with null function pointer deleter");
      static_assert(!std::is_reference<replicator_type>::value || !std::is_rvalue_reference<TRep2>::value, "rvalue replicator bound to reference");
      static_assert(!std::is_reference<deleter_type>::value || !std::is_rvalue_reference<TDel2>::value, "rvalue replicator bound to reference");
    }

    /**
     * Internal state, holding a pointer, a replicator and a deleter
     *
     */
    tuple_type c;
};

/**
 * Swap function overloads for compatible value_ptrs
 *
 * @param x  First value_ptr to swap
 * @param y  Second value_ptr to swap
 */
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &x, value_ptr<T2, R2, D2> &y) noexcept { x.swap(y); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &&x, value_ptr<T2, R2, D2> &y) noexcept { y.swap(std::move(x)); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline void swap(value_ptr<T1, R1, D1> &x, value_ptr<T2, R2, D2> &&y) noexcept { x.swap(std::move(y)); }

/**
 * Equality and difference operator overloads for arbitrary value_ptrs
 *
 * Equality is determined by pointer value.
 *
 *
 * @param x  First value_ptr to compare
 * @param y  Second value_ptr to compare
 * @return the comparison result
 */
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator==(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return x.get() == y.get(); }
template <class T1, class R1, class D1, class T2, class R2, class D2>
inline bool operator!=(value_ptr<T1, R1, D1> const &x, value_ptr<T2, R2, D2> const &y) { return !(x == y); }

/**
 * Equality and difference operator overloads for value_ptrs vs nullptr_t
 *
 * Equality is determined by pointer value.
 *
 *
 * @param x  First value_ptr (nullptr_t) to compare
 * @param y  Second nullptr_t (value_ptr) to compare
 * @return the comparison result
 */
template <class T, class R, class D>
inline bool operator==(value_ptr<T, R, D> const &x, nullptr_t y) { return x.get() == y; }
template <class T, class R, class D>
inline bool operator!=(value_ptr<T, R, D> const &x, nullptr_t y) { return !(x == y); }
template <class T, class R, class D>
inline bool operator==(nullptr_t x, value_ptr<T, R, D> const &y) { return x == y.get(); }
template <class T, class R, class D>
inline bool operator!=(nullptr_t x, value_ptr<T, R, D> const &y) { return !(x == y); }

/**
 * Comparison operators overloads for arbitrary value_ptrs
 *
 * Comparison is determined by pointer value.
 *
 *
 * @param x  First value_ptr to compare
 * @param y  Second value_ptr to compare
 * @return the comparison result
 */
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

#endif /* VALUE_PTR__ */

