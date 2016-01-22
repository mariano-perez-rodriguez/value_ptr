#ifndef VALUE_PTR__CLONEABLE_H__
#define VALUE_PTR__CLONEABLE_H__


#include <type_traits>
#include <cstddef>


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
    static constexpr auto test(R *(S::*)(void *, U...) const, std::nullptr_t)
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
 * Specialization of is_placement_cloneable for fixed array types
 *
 * An array type is never considered placement clonable, since it cannot
 * itself have a "placement clone" method.
 *
 */
template <typename T, std::size_t N>
struct is_placement_cloneable<T[N]> {
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
    static constexpr auto test(R *(S::*)(U...) const, std::nullptr_t)
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
 * Specialization of is_clonable for fixed array types
 *
 * An array type is considered cloneable itself if its constituent type is
 * placement cloneable.
 *
 */
template <typename T, std::size_t N>
struct is_cloneable<T[N]> {
  static constexpr bool value = is_placement_cloneable<T>::value;
};


#endif /* VALUE_PTR__CLONEABLE_H__ */

