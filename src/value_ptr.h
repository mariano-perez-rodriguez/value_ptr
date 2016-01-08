#ifndef VALUE_PTR_H__
#define VALUE_PTR_H__


#include <type_traits>
#include <cstddef>
#include <memory>
#include <tuple>

#include "Handler.h"


/**
 * Smart pointer with value-like semantics
 *
 * @param T  Underlying type to wrap
 * @param H  Handler type to use
 */
template <typename T, typename H = default_handler<T>>
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
     * Export basic type alias for the handler type
     *
     */
    using handler_type            = H;
    using handler_reference       = typename std::add_lvalue_reference<handler_type>::type;
    using handler_const_reference = typename std::add_lvalue_reference<typename std::add_const<handler_type>::type>::type;


  protected:
    /**
     * The internal state will consist of a tuple of:
     *  - a pointer to the underlying type
     *  - a replicator
     *  - a deleter
     *
     */
    using tuple_type = std::tuple<pointer_type, handler_type>;

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
            std::rank<T>::value == 1,
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
    using enable_if_array = std::enable_if<std::rank<U>::value == 1, V>;

  public:
    /**
     * Default constructor
     *
     * Initializes to nullptr delegating to the "master" constructor below.
     *
     */
    constexpr value_ptr() noexcept;
    /**
     * Copy constructor
     *
     * Initializes using the to-be-copied replicator, delegating to the
     * "master" constructor below.
     *
     * @param other  Object to copy
     */
    constexpr value_ptr(value_ptr const &other) noexcept;
    /**
     * Move constructor
     *
     * Initializes using the release()'d pointer, delegating to the
     * "master" constructor below.
     *
     * @param other  Object to move
     */
    constexpr value_ptr(value_ptr &&other) noexcept;

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
    template <typename T2, typename H2> constexpr value_ptr(value_ptr<T2, H2> const &other) noexcept;

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
    template <typename T2, typename H2> constexpr value_ptr(value_ptr<T2, H2> &&other) noexcept;

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
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(T2 *p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(T2 *p, H2&& h) noexcept;

    /**
     * Nullptr constructors
     *
     * These constructors work as the ones above, but they're specialized for
     * the nullptr constant.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param <unnamed>  Nullptr constant
     * @param h  Handler to use
     */
    constexpr value_ptr(nullptr_t) noexcept;
    template <typename H2> constexpr value_ptr(nullptr_t, H&& h) noexcept;

    /**
     * Auto_ptr converting copy-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a replication of an auto_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Auto_ptr to use as replication origin
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(std::auto_ptr<T2> const &p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::auto_ptr<T2> const &p, H2&& h) noexcept;

    /**
     * Auto_ptr converting move-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from an auto_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Auto_ptr to use as pointer origin
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(std::auto_ptr<T2> &&p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::auto_ptr<T2> &&p, H2&& h) noexcept;

    /**
     * Unique_ptr converting copy-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a replication of a unique_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Unique_ptr to use as replication origin
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(std::unique_ptr<T2> const &p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::unique_ptr<T2> const &p, H2&& h) noexcept;

    /**
     * Unique_ptr converting move-constructors
     *
     * These constructors work as the ones above, but take their initial
     * pointer value from a unique_ptr.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Unique_ptr to use as pointer origin
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(std::unique_ptr<T2> &&p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::unique_ptr<T2> &&p, H2&& h) noexcept;

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
     * @param h  Handler to use
     */
    template <typename T2> constexpr value_ptr(std::shared_ptr<T2> const &p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::shared_ptr<T2> const &p, H2&& h) noexcept;

    /**
     * Weak_ptr converting copy-constructors
     *
     * These constructors simply lock a weak_ptr and delegate to the ones given
     * above.
     *
     * They delegate construction to the "master constructor" below.
     *
     * @param p  Weak_ptr to use
     * @param h  Handler to use
     * @param deleter  Deleter to use (when not provided, use unique_ptr's one)
     */
    template <typename T2> constexpr value_ptr(std::weak_ptr<T2> const &p) noexcept;
    template <typename T2, typename H2> constexpr value_ptr(std::weak_ptr<T2> const &p, H2&& h) noexcept;

    /**
     * Nullptr assignment operator
     *
     * This assignment operator specializes the nullptr_t case by simply
     * resetting.
     *
     * @param <unnamed>  Nullptr to assign
     * @return the assigned object
     */
    value_ptr &operator=(nullptr_t) noexcept;

    /**
     * Copy-assignment operator
     *
     * This copy-assignment operator implements the "copy and swap" idiom.
     *
     * @param other  Object to copy-assign
     * @return the assigned object
     */
    value_ptr &operator=(value_ptr other);

    /**
     * Move-assignment operator
     *
     * This Move-assignment operator implements the "move and swap" idiom.
     *
     * @param other  Object to move-assign
     * @return the assigned object
     */
    value_ptr &operator=(value_ptr &&other);

    /**
     * Templated copy-assignment operator
     *
     * This copy-assignment operator accepts any compatible object, it
     * implements the "copy and swap" idiom.
     *
     * @param other  Object to copy-assign
     * @return the assigned object
     */
    template <typename T2, typename H2> typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, H2> other);

    /**
     * Templated move-assignment operator
     *
     * This move-assignment operator accepts any compatible object, it
     * implements the "move and swap" idiom.
     *
     * @param other  Object to move-assign
     * @return the assigned object
     */
    template <typename T2, typename H2> typename enable_if_compatible<T2, value_ptr &>::type operator=(value_ptr<T2, H2> &&other);

    /**
     * Safe bool conversion operator
     *
     * This operator implements a safe bool conversion.
     *
     */
    constexpr operator __unspecified_bool_type() const noexcept;

    /**
     * Virtual destructor
     *
     * The destructor merely resets the object.
     *
     */
    virtual ~value_ptr() noexcept;

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
    template <typename U = T> constexpr typename enable_if_array<U, lvalue_reference_type>::type operator[](std::size_t i) const;

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
    template <typename U = T> typename enable_if_array<U, reference_type>::type operator[](std::size_t i);

    /**
     * Get the pointed-to object
     *
     * @return the pointed-to object as a reference
     */
    constexpr reference_type operator*() const;

    /**
     * Get the current pointer
     *
     * @return the pointer being held
     */
    constexpr pointer_type operator->() const noexcept;

    /**
     * Return the pointer part of the internal state
     *
     * @return the current pointer
     */
    constexpr pointer_type get() const noexcept;

    /**
     * Get a modifiable reference to the current handler
     *
     * @return a reference to the current handler
     */
    handler_reference get_handler() noexcept;

    /**
     * Get an unmodifiable reference to the current handler
     *
     * @return a const reference to the current handler
     */
    constexpr handler_const_reference get_handler() const noexcept;

    /**
     * Release ownership of the current pointer and reset it to nullptr
     *
     * @return the previously owned pointer
     */
    pointer_type release() noexcept;

    /**
     * Reset the internal pointer to the given value (nullptr, by default)
     *
     * @param p  New value to acquire
     */
    void reset(pointer_type p = pointer_type()) noexcept;

    /**
     * Swap the internal state with a compatible value_ptr
     *
     * @param other  The value_ptr to swap values with
     */
    template <typename T2, typename H2> typename enable_if_compatible<T2, void>::type swap(value_ptr<T2, H2> &other) noexcept;

    /**
     * Swap the internal state with a compatible value_ptr (rvalue overload)
     *
     * @param other  The value_ptr to swap values with
     */
    template <typename T2, typename H2> typename enable_if_compatible<T2, void>::type swap(value_ptr<T2, H2> &&other) noexcept;

  protected:
    /**
     * Construct a new value_ptr with the given arguments and perform sanity checks
     *
     * The sanity checks performed are:
     * - if the pointed-to type is polymorphic, the replicator must use clone,
     * - if the handler type is a reference, it cannot be initialized with a temporary.
     *
     *
     * @param p  Pointer to take ownership of
     * @param h  Handler object to use
     * @param <unnamed>  nullptr_t parameter to use for disambiguation
     */
    template <typename T2, typename H2> constexpr value_ptr(T2 *p, H2&& h, typename enable_if_compatible<T2>::type) noexcept;

    /**
     * Construct a new value_ptr from a nullptr
     *
     * The sanity checks performed are:
     * - if the pointed-to type is polymorphic, the replicator must use clone,
     * - if the handler type is a reference, it cannot be initialized with a temporary.
     *
     *
     * @param <unnamed>  Nullptr to use
     * @param h  Handler object to use
     * @param <unnamed>  nullptr_t parameter to use for disambiguation
     */
    template <typename H2> constexpr value_ptr(nullptr_t, H2&& h, nullptr_t) noexcept;

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
template <class T1, class H1, class T2, class H2> inline void swap(value_ptr<T1, H1> &x, value_ptr<T2, H2> &y) noexcept;
template <class T1, class H1, class T2, class H2> inline void swap(value_ptr<T1, H1> &&x, value_ptr<T2, H2> &y) noexcept;
template <class T1, class H1, class T2, class H2> inline void swap(value_ptr<T1, H1> &x, value_ptr<T2, H2> &&y) noexcept;

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
template <class T1, class H1, class T2, class H2> inline bool operator==(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);
template <class T1, class H1, class T2, class H2> inline bool operator!=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);

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
template <class T, class H> inline bool operator==(value_ptr<T, H> const &x, nullptr_t y);
template <class T, class H> inline bool operator!=(value_ptr<T, H> const &x, nullptr_t y);
template <class T, class H> inline bool operator==(nullptr_t x, value_ptr<T, H> const &y);
template <class T, class H> inline bool operator!=(nullptr_t x, value_ptr<T, H> const &y);

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
template <class T1, class H1, class T2, class H2> inline bool operator< (value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);
template <class T1, class H1, class T2, class H2> inline bool operator> (value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);
template <class T1, class H1, class T2, class H2> inline bool operator<=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);
template <class T1, class H1, class T2, class H2> inline bool operator>=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y);


#include "value_ptr.hpp"

#endif /* VALUE_PTR_H__ */

