//##################################################################################################
///
/// @file include/argument.hpp
/// 
/// @brief Implementation of argument entity.
///
//##################################################################################################

#if !defined(CPPARG_ARGUMENT_HPP)
#define CPPARG_ARGUMENT_HPP

//--------------------------------------------------------------------------------------------------
// includes
//--------------------------------------------------------------------------------------------------
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
    /// @brief Concept that restricts argument placeholder types to supported by converter.
    template<typename T>
    concept placeholder_t = requires(T t, const std::string& s) {
        t = converter::convert<T>(s);
    };

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
        struct nargs_t {

            /// @brief Nargs types enumeration.
            enum class nargs_e : unsigned {
            AT_MOST_ONE,
            ANY,
            AT_LEAST_ONE,
        };

            /// @brief Value constructor.
            /// @param type Type of nargs object.
            nargs_t(nargs_e type = nargs_e::AT_MOST_ONE) : type_(type) {
            }


            // Data members
            //
            nargs_e type_;
            unsigned consumedOptCount_;
            unsigned consumedValCount_;
        }; // End: strct nargs_t


    public:

        /// @brief argument constructor
        ///
        /// @param[in]
        ///
        template<typename T>
        requires placeholder_t<T>
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
                 nargs_t                    nargs       = nargs_t(),
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
            // Helper lambda to identify optional arguments (@todo may be redundand)
            auto is_opt = [](const std::string& str) {
                return str.starts_with("--") && str.size() > 2;
            };

            if (!is_opt(*tokens.begin())) {
                throw std::runtime_error("Trying to consume positional argument as optional");
            }
            else {
                consumedOptCount_++;
                tokens.erase(tokens.begin()); // Remove argument name
            }

            for (auto&& token : tokens) {
                if (!is_opt(token)) {
                    action_(std::forward<std::string>(token));
                    consumedValCount_++;
                    tokens.erase(tokens.begin());
                }
                else {
                    // new optional argument encountered
                    break;
                }
            }
        }

        inline void help(std::ostream& out = std::cout) const {
            out << description_;
        }
    
    private:

        static inline bool is_optional(const std::string& str) {
            return str.starts_with("--") && str.size() > 2;
        } // End: is_optional()

        inline bool is_consumed() const {
            // Typically this is the only case where we can say for sure that argument is consumed.
            return (nargs_.type_ == nargs_t::nargs_e::AT_MOST_ONE && nargs_.consumedOptCount_ == 1);
        } // End: is_consumed()


    public:
        std::string                 name_;
        bool                        isOptional_;
        nargs_t                     nargs_;

    protected:

        bool                        isBool_;
        unsigned                    consumedOptCount_;
        unsigned                    consumedValCount_;
        std::string                 error_;

    private:
        action_t                    action_;
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


//--------------------------------------------------------------------------------------------------
// function declarations
//--------------------------------------------------------------------------------------------------
// (none)

#endif // CPPARG_ARGUMENT_HPP

//##################################################################################################
// End of include/argument.hpp
//##################################################################################################
