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
//#include <set> //#include <flat_set>
#include <map>
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

        bool operator<(const argument& other) const {
            return name_ < other.name_;
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


    /// @brief Argument group implementation
    ///
    /// A collection of arguments.
    class group {
    public:

        group(std::string&& id = "default") : id_(std::move(id)) {
        }

        inline void add_argument(argument&& arg) {
            args_.insert({arg.name_, std::move(arg)});
        }

        inline void consume(std::vector<std::string>& tokens) {
            auto arg = args_.find(*tokens.begin());
            if (arg != args_.end()) {
                arg->second.consume(tokens);
            }
        }

    public:
        std::string id_;

    private:
        //std::flat_set<argument> args_;
        std::map<std::string, argument> args_;
    };


    /// @brief  Parser implementation
    ///
    /// Encapsulates positional and optional argument groups.
    class parser {
    public:
        
        parser(std::string&& description = "optional") : positional_("positional"),
                                                         optional_({ description,
                                                                     std::move(description) }) {
        }
        
        void add_argument_group(group&& group) {
            optional_.insert({group.id_, std::move(group)});
        }

        void add_argument(const std::string& groupName,
                          argument&& arg) {
            //auto& target  = (arg.name_)
            //groupName = (groupName.empty() ? "default" : groupName);

        }

        void parse_args(size_t argc, char* argv[]) {
            std::vector<std::string> tokens;
            for (size_t i = 0; i < argc; ++i) {
                tokens.emplace_back(argv[i]);
            }

            // Consume optional arguments first
            std::for_each(optional_.begin(), optional_.end(),
                [&tokens](group& grp) {
                    grp.consume(tokens);
            });

            // Then consume positional arguments
            positional_.consume(tokens);
        }


    private:

        group                           positional_;
        //std::flat_set<group>    optional_;
        std::map<std::string, group>    optional_;
    };

} // namespace argparse


//------------------------------------------------------------------------------
// function declarations
//------------------------------------------------------------------------------
// (none)



/// @todo get rid of junk
//-----------------------------------------------------------------------------------------------------------------------
/*
// some lambda action inspirations
            template<typename T>
            Action(std::string&& name,
                   T& placeholder) : name_(std::move(name))
            {
                if (name == "store")
                {
                    // Also store_default
                    action_ = [&placeholder](const std::string& value)
                    {
                        if (value.empty())
                        {
                            // Special handling for boolean
                            if constexpr (std::is_same_v<T, bool>)
                            {
                                placeholder = Converter::convert<T>("1");
                            }
                        }
                        else
                        {
                            placeholder = Converter::convert<T>(value);
                        }                   
                    };
                }
                else if (name == "count")
                {
                    if constexpr (type::traits::is_incrementable<T>())
                    {
                        action_ = [&placeholder](const std::string& value)
                        {
                            placeholder++;
                        };
                    }
                }
                else
                {
                    // fallthrough so that Action children will be able to extend
                    // the functionality.
                    name_ = "error";
                }        
            } // End: Action::Action()



// const static map in Count class

    class Count {
        private:

            inline static const std::map<std::string, Type> DEFAULTS = {
                { "?", Type::AT_MOST_ONE},
                { "*", Type::ANY},
                { "+", Type::AT_LEAST_ONE},
            };
    
        public:

            Type        type_;
            uint8_t     value_;
        }; // class Count
*/

#endif // CPPARG_ARGUMENT_HPP

//##############################################################################
// End of include/argument.hpp
//##############################################################################
