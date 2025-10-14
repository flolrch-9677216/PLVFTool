#ifndef PLTOOL_FORMULA_H
#define PLTOOL_FORMULA_H

#include <array>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum tokentype
{
    TOK_NOT, // logical NOT
    TOK_AND, // logical AND
    TOK_OR, // logical OR
    TOK_IMP, // logical implication
    TOK_EQ, // logical equivalence
    TOK_XOR, // logical XOR
    TOK_PL, // opening parenthesis (
    TOK_PR, // closing parenthesis )
    TOK_VAR, // variables
    TOK_TRUE, // logical true constant
    TOK_FALSE, // logical false constant
    TOK_IRR // irrelevancy placeholder for forgetTrim
};

class formula
{
    public:
        explicit formula(const char *input);
        ~formula();

        // Query Methods
        [[nodiscard]] bool isValid() const { return _valid; }
        [[nodiscard]] std::string getOriginal() const { return _original; }
        [[nodiscard]] std::string getInvalidReason() const { return _invalid_reason; }
        [[nodiscard]] std::string getUnicodeRepresentation() const;
        [[nodiscard]] std::vector<std::string> getSignature() const { return _signature; }

        // Evaluation Methods
        void evaluate();
        void evaluate(const std::vector<bool> &start, const std::vector<bool> &end);
        bool evaluate(const std::vector<bool> &value);
        bool evaluateRec(const std::vector<bool> &assignment, size_t start, size_t end);
        [[nodiscard]] std::map<std::vector<bool>, bool> getFullEvaluationResults();

        // Transformation Methods
        [[nodiscard]] formula cleanup() const;
        [[nodiscard]] formula forgetClassic(const std::vector<std::string> &vars, bool skepForget) const;
        [[nodiscard]] formula forgetClassicRec(const std::vector<std::string> &vars) const;
        [[nodiscard]] formula forgetSubstitute(const std::vector<std::string> &vars, bool replacement) const;
        [[nodiscard]] formula forgetTrim(const std::vector<std::string> &vars) const;

        // Static Helper Functions
        static void initializeMaps();
        static std::string toUnicode(const std::string &input);

    private:
        // Tokens
        struct token
        {
            std::string label;
            tokentype type;

            token(std::string label, tokentype type) : label(std::move(label)), type(type) {}

            [[nodiscard]] std::string str() const
            {
                switch (type)
                {
                    case TOK_PL:
                        return "(";
                    case TOK_PR:
                        return ")";
                    default:
                        return label;
                }
            }

            [[nodiscard]] int getPrecedence() const
            {
                switch (type)
                {
                    case TOK_EQ:
                        return 1;
                    case TOK_IMP:
                        return 2;
                    case TOK_OR:
                        return 3;
                    case TOK_XOR:
                        return 4;
                    case TOK_AND:
                        return 5;
                    case TOK_NOT:
                        return 6;
                    default:
                        return 0;
                }
            }

            [[nodiscard]] bool isVar() const
            {
                return type == TOK_VAR || type == TOK_TRUE || type == TOK_FALSE;
            }

            [[nodiscard]] bool isOperator() const
            {
                return type == TOK_NOT || type == TOK_AND || type == TOK_OR ||
                       type == TOK_XOR || type == TOK_EQ || type == TOK_IMP;
            }
        };

        // Nested Types
        struct symbol_mapping_t
        {
            const char *unicode;
            std::vector<const char *> shortcuts;
        };

        struct symbol_categories_t
        {
            const char *name;
            std::vector<const char *> symbols;
        };

        explicit formula(const std::vector<token> &tokens);

        // Members
        std::vector<token> _tokens;
        std::vector<std::string> _signature;
        std::map<std::vector<bool>, bool> _evaluation_results;
        std::string _invalid_reason;
        std::string _original;
        bool _valid;
        int _maxdepth = 0;
        int _maxParenthesesID = 0;

        // Static Data
        static const std::vector<symbol_mapping_t> _symbol_mappings;
        static const std::vector<symbol_categories_t> _symbol_categories;
        static std::map<std::string, std::string> _symbolMap;
        static std::map<std::string, tokentype> _operatorMap;
        static constexpr std::array<std::pair<std::string_view, tokentype>, 3> _multiCharOps = {
            {
                {"<->", TOK_EQ},
                {"<=>", TOK_EQ},
                {"->", TOK_IMP}
            }
        };

        // Lexical & Syntax Analysis
        void tokenize();
        void validateTokens();
        [[nodiscard]] std::string reconstructFromTokens() const;
        static bool isBreakCharacter(char c);

        // Evaluation Helpers
        static size_t findMainOp(std::vector<token> &tokens, size_t start, size_t end);
        static std::list<token>::iterator findMainOp(const std::list<token> &tokens,
                                                     const std::list<token>::iterator &start,
                                                     const std::list<token>::iterator &end);

        // Transformation Helpers
        std::vector<token> substitute(const std::string &target, bool replacement);
        static void cleanupRec(std::list<token> &tokens,
                               std::list<token>::iterator start,
                               std::list<token>::iterator end);

        // Variable Forgetting Helpers
        static std::vector<token> forgetClassic(const std::vector<token> &tokens,
                                                const std::set<std::string> &vars,
                                                bool skepForget);
        static void forgetClassicRecRec(std::list<token> &tokens,
                                        std::list<token>::iterator start,
                                        std::list<token>::iterator end,
                                        const std::set<std::string> &vars);
        static void forgetTrimCleanup(std::list<token> &tokens,
                                      std::list<token>::iterator start,
                                      std::list<token>::iterator end);
};

#endif // PLTOOL_FORMULA_H
