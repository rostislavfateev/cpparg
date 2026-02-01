//##############################################################################
///
/// @file include/argument.hpp
/// 
/// @brief Implementation of argument entity.
///
//##############################################################################

#if !defined(CPPARG_ARGUMENT_HPP)
#define CPPARG_ARGUMENT_HPP

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
// system
#include <string>
#include <vector>
#include <functional> // std::function
#include <algorithm>  // 
#include <optional>   // std::optional
#include <iostream>   // std::cout

// user
#include <converter.hpp>


//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------
// (none)


//------------------------------------------------------------------------------
// constants
//------------------------------------------------------------------------------
// (none)


//------------------------------------------------------------------------------
// macros
//------------------------------------------------------------------------------
// (none)


//------------------------------------------------------------------------------
// data types
//------------------------------------------------------------------------------


namespace argparse
{
    /// @brief Argument implementation
    ///
    /// Encapsulates both positional (required) and optional argument types with
    /// arguyment name format (--foo vs foo).
    /// Action lambda must be capturing to allow modification of placeholder;
    /// default is "store".
    /// 
    class argument {
    public:

        /// @brief Represents number of arguments
        enum class nargs_t : unsigned {
            AT_MOST_ONE,
            ANY,
            AT_LEAST_ONE,
        };

        /// @brief Action function type.
        using action_f = std::function<void(const std::string&)>;

    public:

        /// @brief argument constructor
        ///
        /// @param[in]
        ///
        template<typename T>
        argument(std::string&&              name,
                 T&                         placeholder,
                 action_f                   action = [&placeholder](const std::string& value) {
                                                        if (value.empty()) {
                                                            if constexpr (std::is_same_v<T,bool>) {
                                                                placeholder = converter::convert<T>("1");
                                                            }
                                                            else {
                                                                /// @todo apply default maybe?
                                                                //placeholder = defaultVal.value_or(T{});
                                                            }
                                                        }
                                                        else {
                                                            placeholder = converter::convert<T>(value)
                                                        }
                                                    },
                 nargs_t                    nargs       = nargs_t::AT_MOST_ONE,
                 /// @todo can it be the same type as placeholder?
                 //std::string&&              defaultVal  = "",
                 std::optional<T>           defaultVal  = std::nullopt,
                 /// @todo maybe more clever restriction?
                 std::vector<std::string>&& choices     = { },
                 std::string&&              description = "") :
                            action_(std::move(actionName), placeholder),
                            nargs_(nargs),
                            required_(true/*name.contains(--)*/),
                            default_(defaultVal.value_or(T{})),
                            choices_(std::move(choices)),
                            description_(std::move(description)),
                            consumedOptCount_(0),
                            consumedValCount_(0),
                            error_("")
                            isBool_(constexpr (std::is_same_v<T, bool>))
                    // restrict action to be capturing always
                    requires (!std::is_convertible_v<action_f, void(*)()>) {
        }

        /// @todo may be templated by container type?
        inline void consume(std::vector<std::string>& tokens) {
            /// @todo implement
        }

        inline void help(std::ostream& out = std::cout) const {
            out << description_;
        }

    protected:

        bool                        isBool_;
        unsigned                    consumedOptCount_;
        unsigned                    consumedValCount_;
        std::string                 error_;

    private:

        std::string                 name_;
        action_f                    action_;
        nargs_t                     nargs_;
        std::string                 default_;
        std::vector<std::string>    choices_;
        bool                        required_;
        std::string                 description_;
    };

} // namespace argparse


//------------------------------------------------------------------------------
// function declarations
//------------------------------------------------------------------------------
// (none)


#endif // CPPARG_ARGUMENT_HPP

//##############################################################################
// End of include/argument.hpp
//##############################################################################
