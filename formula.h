#ifndef PLTOOL_FORMULA_H
#define PLTOOL_FORMULA_H


#include <list>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <set>

enum tokentype
{
    TOK_NOT, // logical NOT
    TOK_AND, // logical AND
    TOK_OR, // logical OR
    TOK_IMP, // logical implication
    TOK_EQ, // logical equivalence
    TOK_XOR,
    TOK_PL, // opening parenthesis (
    TOK_PR, // closing parenthesis )
    TOK_VAR, // variables
    TOK_TRUE,
    TOK_FALSE,
    TOK_IRR //irrelevancy placeholder for forgetAssumeIrrelevance
};

class formula
{
    public:
        explicit formula(const char *input);

        ~formula();

        [[nodiscard]] bool isValid() const { return _valid; }
        [[nodiscard]] std::string getOriginal() const { return _original; }
        [[nodiscard]] std::string getInvalidReason() const { return _invalid_reason; }

        [[nodiscard]] std::map<std::vector<bool>, bool> getFullEvaluationResults();

        [[nodiscard]] std::string getUnicodeRepresentation() const;

        [[nodiscard]] std::vector<std::string> getSignature() const
        {
            return _signature;
        }

        void evaluate();

        void evaluate(const std::vector<bool> &start, const std::vector<bool> &end);

        bool evaluate(const std::vector<bool> &value);

        [[nodiscard]] formula cleanup() const;

        [[nodiscard]] formula forgetClassic(const std::vector<std::string> &vars, bool skepForget) const;

        [[nodiscard]] formula forgetSubstitute(const std::vector<std::string> &vars, bool skepForget) const;

        [[nodiscard]] formula forgetTrim(const std::vector<std::string> &vars) const;

        [[nodiscard]] formula forgetClassicRec(const std::vector<std::string> &vars) const;


        // Static helper functions
        static std::string toUnicode(const std::string &input);

        static bool isBreakCharacter(char c);

        static void initializeMaps();

    private:
        struct token
        {
            std::string label;
            tokentype type;

            token(std::string label, const tokentype type) : label(std::move(label)), type(type) {}

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
                        return 3; // niedrigste
                    case TOK_XOR:
                        return 4;
                    case TOK_AND:
                        return 5;
                    case TOK_NOT:
                        return 6; // höchste!
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
                return type == TOK_NOT || type == TOK_AND || type == TOK_OR || type == TOK_XOR || type == TOK_EQ || type
                       ==
                       TOK_IMP;
            }
        };

        explicit formula(const std::vector<token> &tokens);

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

        std::vector<token> _tokens;
        std::vector<std::string> _signature;
        std::map<std::vector<bool>, bool> _evaluation_results;
        std::string _invalid_reason;
        bool _valid;
        std::string _original;
        int _maxdepth = 0;
        int _maxParenthesesID = 0;

        static const std::vector<symbol_mapping_t> _symbol_mappings;
        static const std::vector<symbol_categories_t> _symbol_categories;

        static std::map<std::string, std::string> _symbolMap;
        static std::map<std::string, tokentype> _operatorMap;


        void tokenize();

        void validateTokens();

        static std::vector<token> forgetClassic(const std::vector<token> &tokens,
                                                 const std::set<std::string> &vars, bool skepForget);

        static void cleanupRec(std::list<token> &tokens, std::_List_iterator<token> start,
                               std::_List_iterator<token> end);


        static std::list<token>::iterator findMainOp(const std::list<token> &tokens,
                                                              const std::_List_iterator<token> &start,
                                                              const std::_List_iterator<token> &end);

        static void forgetTrimCleanup(std::list<token> &tokens, std::list<token>::iterator start,
                                  std::list<token>::iterator end);

        static void forgetClassicRecRec(std::list<token> &tokens, std::_List_iterator<token> start,
                                        std::_List_iterator<token> end, const std::set<std::string>
                                        &vars);


        static size_t findMainOp(std::vector<token> &tokens, size_t start, size_t end);

        [[nodiscard]] std::string reconstructFromTokens() const;

        [[nodiscard]] int getMaxParenthesesID() const { return _maxParenthesesID; }

        bool evaluateRec(const std::vector<bool> &assignment, size_t start, size_t end);

        std::vector<token> substitute(const std::string &target, bool replacement);
};

#endif //PLTOOL_FORMULA_H
