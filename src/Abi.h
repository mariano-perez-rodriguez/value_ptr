#ifndef VALUE_PTR__ABI_H__
#define VALUE_PTR__ABI_H__


#include <cstdint>


/**
 * Static class to encapsulate ABI dependent operations
 *
 */
class Abi {
  public:
    /**
     * Return the size of the pointed-to array
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     * @return the size of the pointed-to array
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
 * See: https://mentorembedded.github.io/cxx-abi/abi.html
 *
 */
class Itanium : public Abi {
  /**
   * Return the size of the array cookie needed
   *
   * @param T  Underlying type of the array
   * @return the size of the array cookie needed
   */
  template <typename T>
  static constexpr std::size_t arrayCookieLen() noexcept  __attribute__((pure));

  public:
    /**
     * Return the size of the pointed-to array
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     * @return the size of the pointed-to array
     * @throws abi_error  In case the size cannot be determined
     */
    template <typename T>
    static std::size_t arraySize(T const *p) noexcept override __attribute__((pure));

    /**
     * Return a new array, including cookie if needed, but do NOT call constructors
     *
     * @param T  Underlying type of the array
     * @param n  Number of elements in the allocated array
     * @return a pointer to the allocated array
     * @throws std::bad_alloc  In case the underlying operation throws
     */
    template <typename T>
    static T *newArray(std::size_t n) override;

    /**
     * Delete an array created by newArray<T>, including cookie if needed, but do NOT call destructors
     *
     * @param T  Underlying type of the array
     * @param p  Pointer to the array proper
     */
    template <typename T>
    static void delArray(T const *p) noexcept override;
};


#include "Abi.hpp"

#endif /* VALUE_PTR__ABI_H__ */

