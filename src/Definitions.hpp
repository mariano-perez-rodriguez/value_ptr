#ifndef VALUE_PTR__DEFINITIONS__
#define VALUE_PTR__DEFINITIONS__


#include <type_traits>

/**
 * Specialization of conditional that mirrors its bool value to a type
 *
 * @param C  Condition to "lift" to a type
 */
template <bool C>
using condition = std::conditional<C, std::true_type, std::false_type>;


#endif /* VALUE_PTR__DEFINITIONS__ */

