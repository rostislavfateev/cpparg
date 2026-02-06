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
#include <deque>
#include <flat_map>
#include <map>
#include <functional> // std::function
#include <algorithm>  // 
#include <optional>   // std::optional
#include <variant>    // std::variant
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

        /// @brief Action function type.
        using action_t = std::function<void(const std::string&)>;

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
                 action_t                   action = [&placeholder](const std::string& value) {
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
                            action_(action),
                            nargs_(nargs),
                            isOptional_(is_optional(name)),
                            default_(defaultVal.value_or(T{})),
                            choices_(std::move(choices)),
                            description_(std::move(description)),
                            error_(""),
                            isBool_(constexpr (std::is_same_v<T, bool>)) {
        }

        bool operator<(const argument& other) const {
            return name_ < other.name_;
        }

        inline void consume(std::deque<std::string>& tokens) {
        /// @todo Problem: better define stopping criteria especially for list arguments,
        /// as now it stops if consumed or at next optional argument detection.
            while (!tokens.empty()
                   && (!is_consumed() || !is_optional(*tokens.begin()))) {
                action_(std::forward<std::string>(*tokens.begin()));
                nargs_.consumedValCount_++;
                tokens.pop_front();
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
        std::string                 error_;

    private:
        action_t                    action_;
        std::string                 default_;
        std::vector<std::string>    choices_;
        std::string                 description_;
    };


    /// @brief Argument group implementation
    ///
    /// A collection of arguments.
    class group {

        using container_variant = std::variant<std::deque<argument>, std::map<std::string, argument>>;
    public:

        group(std::string&& id) : id_(std::move(id)) {
        }

        inline void add_argument(argument&& arg) {
            if (arg.isOptional_) {
                optional_.insert({arg.name_, std::move(arg)});
        }
            else {
                positional_.emplace_back(std::move(arg));
            }
        }

        inline void consume_optional(std::deque<std::string>& tokens) {
            auto arg = optional_.find(*tokens.begin());
            if (arg != optional_.end()) {
                // strip optional argument of its name.
                tokens.pop_front();
                arg->second.nargs_.consumedOptCount_++;
                arg->second.consume(tokens);
            }
        }

        inline void consume_positional(std::deque<std::string>& tokens) {
            for (auto& arg : postional_) {
                arg.consume(tokens);
            }
        }


    public:
        std::string id_;

    private:
        //std::flat_map<argument> args_;
        std::deque<argument>            positional_;
        std::map<std::string, argument> optional_;
    };


    /// @brief  Parser implementation
    ///
    /// Encapsulates positional and optional argument groups.
    class parser {
    public:
        
        parser(std::string&& description = "optional") : groups_({"default", {std::move("default")}}),
                                                         description_(std::move(description)) {
        }
        
        void add_argument_group(group&& group) {
            groups_.insert({group.id_, std::move(group)});
        }

        void add_argument(argument&& arg,
                          const std::string& groupName = "default") {
            auto group = groups_.find(groupName);
            if (group != groups_.end()) {
                group->second.add_argument(std::move(arg));
            }
            else {
                throw std::runtime_error("Argument group not found: " + groupName);
            }
        }

        void parse_args(size_t argc, char* argv[]) {
            std::deque<std::string> tokens;
            for (size_t i = 0; i < argc; ++i) {
                tokens.emplace_back(argv[i]);
            }

            // Consume optional arguments first
            std::for_each(groups_.begin(), groups_.end(),
                [&tokens](group& grp) {
                    grp.consume_optional(tokens);
            });

            // Then consume positional arguments
            std::for_each(groups_.begin(), groups_.end(),
                [&tokens](group& grp) {
                    grp.consume_positional(tokens);
            });
        }


    private:

        //std::flat_set<group>    optional_;
        std::map<std::string, group>    groups_;
        std::string                     description_;
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
