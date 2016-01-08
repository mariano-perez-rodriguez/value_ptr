#ifndef VALUE_PTR__
#define VALUE_PTR__


#include "value_ptr.h"

#include <functional>


/**
 * Default constructor
 *
 * Initializes to nullptr delegating to the "master" constructor below.
 *
 */
template <typename T, typename H>
constexpr value_ptr<T, H>::value_ptr() noexcept : value_ptr<T, H>{pointer_type(), handler_type(), nullptr} {}

/**
 * Copy constructor
 *
 * Initializes using the to-be-copied replicator, delegating to the
 * "master" constructor below.
 *
 * @param other  Object to copy
 */
template <typename T, typename H>
constexpr value_ptr<T, H>::value_ptr(value_ptr<T, H> const &other) noexcept : value_ptr<T, H>{other.get_handler().replicate(other.get()), other.get_handler(), nullptr} {}

/**
 * Move constructor
 *
 * Initializes using the release()'d pointer, delegating to the
 * "master" constructor below.
 *
 * @param other  Object to move
 */
template <typename T, typename H>
constexpr value_ptr<T, H>::value_ptr(value_ptr<T, H> &&other) noexcept : value_ptr<T, H>{other.release(), std::move(other.get_handler()), nullptr} {}

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
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(typename value_ptr<T, H>::template enable_if_different<T2, value_ptr<T2, H2>>::type const &other) noexcept : value_ptr<T, H>{other.get_handler().replicate(other.get()), other.get_handler(), nullptr} {}

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
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(typename value_ptr<T, H>::template enable_if_different<T2, value_ptr<T2, H2>>::type &&other) noexcept : value_ptr<T, H>{other.release(), std::move(other.get_handler()), nullptr} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(T2 *p) noexcept : value_ptr<T, H>{p, handler_type(), nullptr} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(T2 *p, H2&& h) noexcept : value_ptr<T, H>{p, std::forward<H2>(h), nullptr} {}

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
template <typename T, typename H>
constexpr value_ptr<T, H>::value_ptr(nullptr_t) noexcept : value_ptr<T, H>{nullptr, handler_type(), nullptr} {}
template <typename T, typename H>
template <typename H2>
constexpr value_ptr<T, H>::value_ptr(nullptr_t, H&& h) noexcept : value_ptr<T, H>{nullptr, std::forward<H2>(h), nullptr} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::auto_ptr<T2> const &p) noexcept : value_ptr<T, H>{handler_type().replicate(p.get())} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::auto_ptr<T2> const &p, H2&& h) noexcept : value_ptr<T, H>{h.replicate(p.get()), std::forward<H2>(h)} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::auto_ptr<T2> &&p) noexcept : value_ptr<T, H>{p.release()} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::auto_ptr<T2> &&p, H2&& h) noexcept : value_ptr<T, H>{p.release(), std::forward<H2>(h)} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::unique_ptr<T2> const &p) noexcept : value_ptr<T, H>{handler_type().replicate(p.get()), handler_type()} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::unique_ptr<T2> const &p, H2&& h) noexcept : value_ptr<T, H>{h.replicate(p.get()), std::forward<H2>(h)} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::unique_ptr<T2> &&p) noexcept : value_ptr<T, H>{p.release(), handler_type()} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::unique_ptr<T2> &&p, H2&& h) noexcept : value_ptr<T, H>{p.release(), std::forward<H2>(h)} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::shared_ptr<T2> const &p) noexcept : value_ptr<T, H>{handler_type().replicate(p.get()), handler_type()} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::shared_ptr<T2> const &p, H2&& h) noexcept : value_ptr<T, H>{handler_type().replicate(p.get()), std::forward<H2>(h)} {}

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
template <typename T, typename H>
template <typename T2>
constexpr value_ptr<T, H>::value_ptr(std::weak_ptr<T2> const &p) noexcept : value_ptr<T, H>{p.lock()} {}
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(std::weak_ptr<T2> const &p, H2&& h) noexcept : value_ptr<T, H>{p.lock(), std::forward<H2>(h)} {}

/**
 * Nullptr assignment operator
 *
 * This assignment operator specializes the nullptr_t case by simply
 * resetting.
 *
 * @param <unnamed>  Nullptr to assign
 * @return the assigned object
 */
template <typename T, typename H>
value_ptr<T, H> &value_ptr<T, H>::operator=(nullptr_t) noexcept { reset(); return *this; }

/**
 * Copy-assignment operator
 *
 * This copy-assignment operator DOES NOT implement the "copy and swap" idiom ON PURPOSE (ambiguous declarations otherwise).
 *
 * @param other  Object to copy-assign
 * @return the assigned object
 */
template <typename T, typename H>
value_ptr<T, H> &value_ptr<T, H>::operator=(value_ptr<T, H> const &other) {
  value_ptr<T, H> tmp = other;
  swap(tmp);
  return *this;
}

/**
 * Move-assignment operator
 *
 * This Move-assignment operator implements the "move and swap" idiom.
 *
 * @param other  Object to move-assign
 * @return the assigned object
 */
template <typename T, typename H>
value_ptr<T, H> &value_ptr<T, H>::operator=(value_ptr<T, H> &&other) { swap(std::move(other)); return *this; }

/**
 * Templated copy-assignment operator
 *
 * This copy-assignment operator accepts any compatible object, it
 * DOES NOT implement the "copy and swap" idiom ON PURPOSE (ambiguous declarations otherwise).
 *
 * @param other  Object to copy-assign
 * @return the assigned object
 */
template <typename T, typename H>
template <typename T2, typename H2>
typename value_ptr<T, H>::template enable_if_compatible<T2, value_ptr<T, H> &>::type value_ptr<T, H>::operator=(typename value_ptr<T, H>::template enable_if_different<T2, value_ptr<T2, H2>>::type const &other) {
  value_ptr<T, H> tmp = other;
  swap(other);
  return *this;
}

/**
 * Templated move-assignment operator
 *
 * This move-assignment operator accepts any compatible object, it
 * implements the "move and swap" idiom.
 *
 * @param other  Object to move-assign
 * @return the assigned object
 */
template <typename T, typename H>
template <typename T2, typename H2>
typename value_ptr<T, H>::template enable_if_compatible<T2, value_ptr<T, H> &>::type value_ptr<T, H>::operator=(typename value_ptr<T, H>::template enable_if_different<T2, value_ptr<T2, H2>>::type &&other) { swap(std::move(other)); return *this; }

/**
 * Safe bool conversion operator
 *
 * This operator implements a safe bool conversion.
 *
 */
template <typename T, typename H>
constexpr value_ptr<T, H>::operator value_ptr<T, H>::__unspecified_bool_type() const noexcept { return nullptr == get() ? nullptr : &value_ptr<T, H>::c; }

/**
 * Virtual destructor
 *
 * The destructor merely resets the object.
 *
 */
template <typename T, typename H>
value_ptr<T, H>::~value_ptr() noexcept { reset(); }

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
template <typename T, typename H>
template <typename U>
constexpr typename value_ptr<T, H>::template enable_if_array<U, typename value_ptr<T, H>::lvalue_reference_type>::type value_ptr<T, H>::operator[](std::size_t i) const { return get()[i]; }

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
template <typename T, typename H>
template <typename U>
typename value_ptr<T, H>::template enable_if_array<U, typename value_ptr<T, H>::reference_type>::type value_ptr<T, H>::operator[](std::size_t i) { return get()[i]; }

/**
 * Get the pointed-to object
 *
 * @return the pointed-to object as a reference
 */
template <typename T, typename H>
constexpr typename value_ptr<T, H>::reference_type value_ptr<T, H>::operator*() const { return *get(); }

/**
 * Get the current pointer
 *
 * @return the pointer being held
 */
template <typename T, typename H>
constexpr typename value_ptr<T, H>::pointer_type value_ptr<T, H>::operator->() const noexcept { return get(); }

/**
 * Return the pointer part of the internal state
 *
 * @return the current pointer
 */
template <typename T, typename H>
constexpr typename value_ptr<T, H>::pointer_type value_ptr<T, H>::get() const noexcept { return std::get<0>(c); }

/**
 * Get a modifiable reference to the current handler
 *
 * @return a reference to the current handler
 */
template <typename T, typename H>
typename value_ptr<T, H>::handler_reference value_ptr<T, H>::get_handler() noexcept { return std::get<1>(c); }

/**
 * Get an unmodifiable reference to the current handler
 *
 * @return a const reference to the current handler
 */
template <typename T, typename H>
constexpr typename value_ptr<T, H>::handler_const_reference value_ptr<T, H>::get_handler() const noexcept { return std::get<1>(c); }

/**
 * Release ownership of the current pointer and reset it to nullptr
 *
 * @return the previously owned pointer
 */
template <typename T, typename H>
typename value_ptr<T, H>::pointer_type value_ptr<T, H>::release() noexcept { value_ptr<T, H>::pointer_type old = get(); std::get<0>(c) = nullptr; return old; }

/**
 * Reset the internal pointer to the given value (nullptr, by default)
 *
 * @param p  New value to acquire
 */
template <typename T, typename H>
void value_ptr<T, H>::reset(typename value_ptr<T, H>::pointer_type p) noexcept { if (p != get()) { get_handler().destroy(get()); std::get<0>(c) = p; } }

/**
 * Swap the internal state with a compatible value_ptr
 *
 * @param other  The value_ptr to swap values with
 */
template <typename T, typename H>
template <typename T2, typename H2>
typename value_ptr<T, H>::template enable_if_compatible<T2, void>::type value_ptr<T, H>::swap(value_ptr<T2, H2> &other) noexcept { using std::swap; swap(c, other.c); }

/**
 * Swap the internal state with a compatible value_ptr (rvalue overload)
 *
 * @param other  The value_ptr to swap values with
 */
template <typename T, typename H>
template <typename T2, typename H2>
typename value_ptr<T, H>::template enable_if_compatible<T2, void>::type value_ptr<T, H>::swap(value_ptr<T2, H2> &&other) noexcept { using std::swap; swap(c, other.c); }

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
 * @param h  Handler object to use
 * @param <unnamed>  nullptr_t parameter to use for disambiguation
 */
template <typename T, typename H>
template <typename T2, typename H2>
constexpr value_ptr<T, H>::value_ptr(T2 *p, H2&& h, typename value_ptr<T, H>::template enable_if_compatible<T2>::type) noexcept : c{p, std::forward<H2>(h)} {
  static_assert(!std::is_polymorphic<T>::value || H::slice_safe, "would slice when copying");
  static_assert(!std::is_reference<value_ptr<T, H>::handler_type>::value || !std::is_rvalue_reference<H2>::value, "rvalue handler bound to reference");
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
 * @param h  Handler object to use
 * @param <unnamed>  nullptr_t parameter to use for disambiguation
 */
template <typename T, typename H>
template <typename H2>
constexpr value_ptr<T, H>::value_ptr(nullptr_t, H2&& h, nullptr_t) noexcept : c{nullptr, std::forward<H2>(h)} {
  static_assert(!std::is_polymorphic<T>::value || H::slice_safe, "would slice when copying");
  static_assert(!std::is_reference<value_ptr<T, H>::handler_type>::value || !std::is_rvalue_reference<H2>::value, "rvalue handler bound to reference");
}



/**
 * Swap function overloads for compatible value_ptrs
 *
 * @param x  First value_ptr to swap
 * @param y  Second value_ptr to swap
 */
template <class T1, class H1, class T2, class H2>
inline void swap(value_ptr<T1, H1> &x, value_ptr<T2, H2> &y) noexcept { x.swap(y); }
template <class T1, class H1, class T2, class H2>
inline void swap(value_ptr<T1, H1> &&x, value_ptr<T2, H2> &y) noexcept { y.swap(std::move(x)); }
template <class T1, class H1, class T2, class H2>
inline void swap(value_ptr<T1, H1> &x, value_ptr<T2, H2> &&y) noexcept { x.swap(std::move(y)); }

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
template <class T1, class H1, class T2, class H2>
inline bool operator==(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) { return x.get() == y.get(); }
template <class T1, class H1, class T2, class H2>
inline bool operator!=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) { return !(x == y); }

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
template <class T, class H>
inline bool operator==(value_ptr<T, H> const &x, nullptr_t y) { return x.get() == y; }
template <class T, class H>
inline bool operator!=(value_ptr<T, H> const &x, nullptr_t y) { return !(x == y); }
template <class T, class H>
inline bool operator==(nullptr_t x, value_ptr<T, H> const &y) { return x == y.get(); }
template <class T, class H>
inline bool operator!=(nullptr_t x, value_ptr<T, H> const &y) { return !(x == y); }

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
template <class T1, class H1, class T2, class H2>
inline bool operator< (value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) {
  using CT = typename std::common_type<typename value_ptr<T1, H1>::pointer_type, typename value_ptr<T2, H2>::pointer_type>::type;
  return std::less<CT>()(x.get(), y.get());
}
template <class T1, class H1, class T2, class H2>
inline bool operator> (value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) { return y < x; }
template <class T1, class H1, class T2, class H2>
inline bool operator<=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) { return !(y < x); }
template <class T1, class H1, class T2, class H2>
inline bool operator>=(value_ptr<T1, H1> const &x, value_ptr<T2, H2> const &y) { return !(x < y); }


#endif /* VALUE_PTR__ */

