#ifndef VALUE_PTR__HANDLER__
#define VALUE_PTR__HANDLER__


#include <type_traits>
#include <exception>
#include <new>

#include "Abi.hpp"
#include "Cloneable.hpp"

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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> const &) noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_copy &operator=(default_copy<U, ABI> &&) noexcept { return *this; }
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> const &) noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_copy &operator=(default_copy<U, ABI> &&) noexcept { return *this; }
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
 * Specialization of default_copy for fixed array types
 *
 */
template <typename T, typename ABI, std::size_t N>
struct default_copy<T[N], ABI> {
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_copy() noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> const &) noexcept {}
  template <typename U> default_copy(default_copy<U, ABI> &&) noexcept {}
  template <typename U> default_copy &operator=(default_copy<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_copy &operator=(default_copy<U, ABI> &&) noexcept { return *this; }
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

    std::size_t i;
    T *ret = ABI::template newArray<T>(N);

    try {
      for (i = 0; i < N; i++) {
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> const &) noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_clone &operator=(default_clone<U, ABI> &&) noexcept { return *this; }
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> const &) noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_clone &operator=(default_clone<U, ABI> &&) noexcept { return *this; }
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
 * Specialization of default_clone for fixed array types
 *
 */
template <typename T, typename ABI, std::size_t N>
struct default_clone<T[N], ABI> {
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_clone() noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> const &) noexcept {}
  template <typename U> default_clone(default_clone<U, ABI> &&) noexcept {}
  template <typename U> default_clone &operator=(default_clone<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_clone &operator=(default_clone<U, ABI> &&) noexcept { return *this; }
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

    std::size_t i;
    T *ret = ABI::template newArray<T>(N);

    try {
      for (i = 0; i < N; i++) {
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
   * Note: only accept the same ABI counterparts.
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
   * Note: only accept the same ABI counterparts.
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
   * Note: only accept the same ABI counterparts.
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
   * Note: only accept the same ABI counterparts.
   *
   */
  default_destroy() noexcept {}
  template <typename U> default_destroy(default_destroy<U, ABI> const &) noexcept {}
  template <typename U> default_destroy(default_destroy<U, ABI> &&) noexcept {}
  template <typename U> default_destroy &operator=(default_destroy<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_destroy &operator=(default_destroy<U, ABI> &&) noexcept { return *this; }
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
 * Specialization of default_destroy for fixed array types
 *
 */
template <typename T, typename ABI, std::size_t N>
struct default_destroy<T[N], ABI> {
  /**
   * Interaction boilerplate
   *
   * This battery of definitions are in place so that objects may be copied,
   * moved, initialized, and assigned arbitrarily. Note that this is not a big
   * deal, since this structure itself has no members.
   *
   * Note: only accept the same ABI counterparts.
   *
   */
  default_destroy() noexcept {}
  template <typename U> default_destroy(default_destroy<U, ABI> const &) noexcept {}
  template <typename U> default_destroy(default_destroy<U, ABI> &&) noexcept {}
  template <typename U> default_destroy &operator=(default_destroy<U, ABI> const &) noexcept { return *this; }
  template <typename U> default_destroy &operator=(default_destroy<U, ABI> &&) noexcept { return *this; }
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

    std::size_t i = N;

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


#endif /* VALUE_PTR__HANDLER__ */

