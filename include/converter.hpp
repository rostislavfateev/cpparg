//##################################################################################################
///
/// @file include/converter.hpp
/// 
/// @brief Implementation of converter entity.
///
//##################################################################################################

#if !defined(CPPARG_CONVERTER_HPP)
#define CPPARG_CONVERTER_HPP

//--------------------------------------------------------------------------------------------------
// includes
//--------------------------------------------------------------------------------------------------
// system
#include <string>
#include <stdexcept>
#include <vector>
#include <ranges> // std::views
#include <type_traits>


//--------------------------------------------------------------------------------------------------
// forward declarations
//--------------------------------------------------------------------------------------------------
// (none)


//--------------------------------------------------------------------------------------------------
// constants
//--------------------------------------------------------------------------------------------------
// (none)


//--------------------------------------------------------------------------------------------------
// macros
//--------------------------------------------------------------------------------------------------
// (none)


//--------------------------------------------------------------------------------------------------
// data types
//--------------------------------------------------------------------------------------------------

namespace argparse
{
    namespace type
    {
        namespace traits
        {
            /// @brief Always false type trait for static_asserts in templates.
            template <class...>
            constexpr std::false_type always_false {};


        /// @todo replace with std::is_constructible in C++23
            /// @brief Type trait to allow CustomType(const std::string&) user-defined types.
            template<typename T, typename = void>
            struct is_string_constructible : std::false_type
            {};

            template<typename T>
            struct is_string_constructible<T, std::void_t<decltype(T(std::declval<std::string>()))>> : std::true_type
            {};


            /// @brief Type trait to check if T is incrementable (supports ++T and T++. 
            template<typename T, typename = void>
            struct is_incrementable : std::false_type
            {};

            template<typename T>
            struct is_incrementable<T, std::void_t<decltype(++std::declval<T&>(),
                                                            std::declval<T&>()++)>> : std::true_type
            {};


            /// @brief Check if T is a standard container (vector, list, set, etc.)
            template<typename T, typename = void>
            struct is_container : std::false_type
            {};

            template<typename T>
            struct is_container<T, std::void_t<typename T::value_type,
                                                typename T::iterator,
                                                decltype(std::declval<T>().begin()),
                                                decltype(std::declval<T>().end())>> : std::true_type
            {};

            /// @brief Extract element type from container
            template<typename T>
            struct container_element_type {
                using type = typename T::value_type;
            };
        } // namespace traits
    } // namespace type
}


namespace argparse
{
    class converter
    {
    public:

        template<typename T>
        static T convert(const std::string& value)
        {
            if (value.empty())
            {
                return T{};
            }

            // Handle containers (vector, list, set, etc.)
            if      constexpr (type::traits::is_container<T>()) {
                return parse_container<T>(value);
            }
            // Handle scalar types
            else if constexpr (std::is_same_v<T, std::string>) {
                return value;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return value == "1" || value == "true";
            }
            else if constexpr (std::is_same_v<T, float>) {
                return std::stof(value);
            }
            else if constexpr (std::is_same_v<T, double>) {
                return std::stod(value);
            }
            else if constexpr (std::is_same_v<T, int>) {
                return std::stoi(value);
            }
            else if constexpr (std::is_same_v<T, unsigned int>) {
                return unsigned(std::stoul(value));
            }
            else if constexpr (std::is_same_v<T, long int>) {
                return std::stoll(value);
            }
            else if constexpr (std::is_same_v<T, unsigned long int>) {
                return std::stoull(value);
            }
            else if constexpr (type::traits::is_string_constructible<T>()) {
                return T(value);
            }
            else {
                static_assert(type::traits::always_false<T>, "Unsupported type");
            }
        }

    private:

        template<typename Container>
        static Container parse_container(const std::string& value)
        {
            using ElementType = typename type::traits::container_element_type<Container>::type;
            Container result;
            
            for (auto&& part : std::views::split(value, ' ')) {
                std::string token(part.begin(), part.end());
                if (!token.empty()) {
                    result.insert(result.end(),
                                  convert<ElementType>(token));
                }
            }
            
            return result;
        }
    };
}


#endif // !defined(CPPARG_CONVERTER_HPP)

//##################################################################################################
// End of include/converter.hpp
//##################################################################################################
