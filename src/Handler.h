#ifndef VALUE_PTR__HANDLER_H__
#define VALUE_PTR__HANDLER_H__


#include <type_traits>

#include "Abi.h"
#include "Cloneable.h"


/**
 * Metaprogramming class to automatically select the destruction method to use
 *
 * @param T  Class to select a destruction method for
 * @param ABI  ABI adapter class to use
 */
template <typename T, typename ABI>
struct default_destroy {
  /**
   * Destroyer implementation
   *
   * This method simply applies delete to the given pointer.
   *
   * @param p  Pointer to the object to delete
   */
  void destroy(T const *p) const;
};

/**
 * Specialization of default_destroy for array types
 *
 */
template <typename T, typename ABI>
struct default_destroy<T[], ABI> {
  /**
   * Destroyer implementation
   *
   * This method calls each object's destructor and then deletes the substrate
   * array.
   *
   * @param p  Pointer to the array to delete
   */
  void destroy(T const *p) const;
};

/**
 * Specialization of default_destroy for fixed array types
 *
 */
template <typename T, typename ABI, std::size_t N>
struct default_destroy<T[N], ABI> {
  /**
   * Destroyer implementation
   *
   * This method calls each object's destructor and then deletes the substrate
   * array.
   *
   * @param p  Pointer to the array to delete
   */
  void destroy(T const *p) const;
};



/**
 * Metaprogramming class to provide a default replicator using the class' copy constructor
 *
 * @param T  Class to provide a replicator for
 * @param ABI  ABI adapter class to use
 */
template <typename T, typename ABI>
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
   * Replication implementation
   *
   * This method returns a new object of the underlying class by calling its
   * copy constructor on the given object, it returns nullptr if a nullptr is
   * given.
   *
   * @param p  Pointer to the object to copy
   * @return either nullptr if nullptr is given, or a new object copied from p
   */
  T *replicate(T const *p) const;
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
   * Replication implementation
   *
   * This method returns a new array of objects of the underlying class by
   * performing a placement new using its copy constructor on each given
   * object, it returns nullptr if a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array copied from p
   */
  T *replicate(T const *p) const;
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
   * Replication implementation
   *
   * This method returns a new array of objects of the underlying class by
   * performing a placement new using its copy constructor on each given
   * object, it returns nullptr if a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array copied from p
   */
  T *replicate(T const *p) const;
};



/**
 * Metaprogramming class to provide a default replicator using the class' "clone" method
 *
 * @param T  Class to provide a replicator for
 * @param ABI  ABI adapter class to use
 */
template <typename T, typename ABI>
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
   * Replication implementation
   *
   * This method returns a new object of the underlying class by calling its
   * "clone" method on the given object, it returns nullptr if a nullptr is
   * given.
   *
   * @param p  Pointer to the object to copy
   * @return either nullptr if nullptr is given, or a new object cloned from p
   */
  T *replicate(T const *p) const;
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
   * Replication implementation
   *
   * This method returns a new array of objects of the underlying class by
   * performing a placement clone on each given object, it returns nullptr if
   * a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array cloned from p
   */
  T *replicate(T const *p) const;
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
   * Replication implementation
   *
   * This method returns a new array of objects of the underlying class by
   * performing a placement clone on each given object, it returns nullptr if
   * a nullptr is given.
   *
   * @param p  Pointer to the array to copy
   * @return either nullptr if nullptr is given, or a new array cloned from p
   */
  T *replicate(T const *p) const;
};



/**
 * Metaprogramming class to provide a default replicator
 *
 * @param T  Class to provide a replicator for
 * @param ABI  ABI adapter class to use
 * @param use_clone  Whether to use a "clone" method or not (defaults to automatic detection)
 */
template <typename T, typename ABI, bool use_clone = is_cloneable<T>::value> struct default_replicate;

/**
 * Specialization of default_replicate for "clone" method usage
 *
 */
template <typename T, typename ABI>
struct default_replicate<T, ABI, true> : public default_clone<T, ABI> {
  using default_clone<T, ABI>::replicate;

  /**
   * Whether the replication method uses "clone" methods
   *
   */
  static constexpr bool slice_safe = true;
};

/**
 * Specialization of default_replicate for copy-constructor usage
 *
 */
template <typename T, typename ABI>
struct default_replicate<T, ABI, false> : public default_copy<T, ABI> {
  using default_copy<T, ABI>::replicate;

  /**
   * Whether the replication method uses "clone" methods
   *
   */
  static constexpr bool slice_safe = false;
};



/**
 * Metaprogramming class encapsulating replication and destruction
 *
 * @param T  Underlying type this class handles
 * @param ABI  ABI adapter class to use (Itanium by default)
 * @param use_clone  Whether the replication should use cloning or copying
 */
template <typename T, typename ABI = Itanium>
struct default_handler : public default_destroy<T, ABI>, public default_replicate<T, ABI> {
  using default_destroy<T, ABI>::destroy;
  using default_replicate<T, ABI>::replicate;
  using default_replicate<T, ABI>::slice_safe;
};


#include "Handler.hpp"

#endif /* VALUE_PTR__HANDLER_H__ */

