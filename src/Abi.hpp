#ifndef VALUE_PTR__ABI_HPP__
#define VALUE_PTR__ABI_HPP__


#include "Abi.h"

#include <algorithm>


/**
 * Return the size of the array cookie needed
 *
 * @param T  Underlying type of the array
 * @return the size of the array cookie needed
 */
template <typename T>
constexpr std::size_t Itanium::arrayCookieLen() noexcept {
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
std::size_t Itanium::arraySize(T const *p) noexcept {
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
T *Itanium::newArray(std::size_t n) {
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
void Itanium::delArray(T const *p) noexcept {
  delete[] (reinterpret_cast<char const *>(p) - arrayCookieLen<T>());
}

#endif /* VALUE_PTR__ABI_HPP__ */

