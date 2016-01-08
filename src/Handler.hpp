#ifndef VALUE_PTR__HANDLER_HPP__
#define VALUE_PTR__HANDLER_HPP__


#include "Handler.h"

#include <exception>
#include <cstdint>
#include <new>


/**
 * Destroyer implementation
 *
 * This method simply applies delete to the given pointer.
 *
 * @param p  Pointer to the object to delete
 */
template <typename T, typename ABI>
void default_destroy<T, ABI>::destroy(T const *p) const {
  static_assert(sizeof(T) > 0, "default_destroy cannot work on incomplete types");

  delete p;
}

/**
 * Destroyer implementation
 *
 * This method calls each object's destructor and then deletes the substrate
 * array.
 *
 * @param p  Pointer to the array to delete
 */
template <typename T, typename ABI>
void default_destroy<T[], ABI>::destroy(T const *p) const {
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

/**
 * Destroyer implementation
 *
 * This method calls each object's destructor and then deletes the substrate
 * array.
 *
 * @param p  Pointer to the array to delete
 */
template <typename T, typename ABI, std::size_t N>
void default_destroy<T[N], ABI>::destroy(T const *p) const {
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
template <typename T, typename ABI>
T *default_copy<T, ABI>::replicate(T const *p) const {
  return nullptr != p ? new T{*p} : nullptr;
}

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
template <typename T, typename ABI>
T *default_copy<T[], ABI>::replicate(T const *p) const {
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
template <typename T, typename ABI, std::size_t N>
T *default_copy<T[N], ABI>::replicate(T const *p) const {
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
template <typename T, typename ABI>
T *default_clone<T, ABI>::replicate(T const *p) const {
  return nullptr != p ? p->clone() : nullptr;
}

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
template <typename T, typename ABI>
T *default_clone<T[], ABI>::replicate(T const *p) const {
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
template <typename T, typename ABI, std::size_t N>
T *default_clone<T[N], ABI>::replicate(T const *p) const {
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


#endif /* VALUE_PTR__HANDLER_HPP__ */

