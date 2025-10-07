#include "formula.h"
#include <cstring>
#include <cctype>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <iostream>
#include <set>
#include <stack>
#include <utility>
#include <climits>
#include <list>

const std::vector<formula::symbol_mapping_t> formula::_symbol_mappings = {
    {"∧", {"and", "&", "&&", "land"}},
    {"∨", {"or", "|", "||", "lor"}},
    {"⊕", {"xor", "^"}},
    {"¬", {"not", "!", "~", "neg"}},
    {"→", {"implies", "imp", "->", "rightarrow", ">"}},
    {"↔", {"iff", "=", "<->", "<=>", "leftrightarrow", "eq", "equals"}},
    {"⊤", {"true", "top", "tau", "tautology"}},
    {"⊥", {"false", "bot", "bottom", "con", "contradiction"}},
    // {"⊨", {"models", "|=", "entails", "vDash"}},
    // {"≡", {"equiv", "==", "==="}},
    // {"∀", {"forall"}},
    // {"∃", {"exists"}},
    {"φ", {"phi", "varphi"}},
    {"ψ", {"psi"}},
    {"χ", {"chi"}}
};

const std::vector<formula::symbol_categories_t> formula::_symbol_categories = {
    {"Logical Operators", {"∧", "∨", "¬", "→", "↔", "⊕"}},
    {"Constants", {"⊤", "⊥"}},
    // {"Relations", {"⊨", "≡", "⊕"}},
    // {"Quantifiers", {"∀", "∃"}},
    {"Greek Letters", {"φ", "ψ", "χ"}}
};

std::map<std::string, std::string> formula::_symbolMap;
std::map<std::string, tokentype> formula::_operatorMap;

formula::formula(const char *input)
{
    _original = input;
    _valid = false;

    initializeMaps();

    //lexikalische analyse, input string zu Tokens machen
    tokenize();

    validateTokens();
}

formula::formula(const std::vector<token> &tokens)
{
    _tokens = tokens;
    _original = reconstructFromTokens();
    _valid = false;

    initializeMaps();
    validateTokens();
}

formula::~formula() = default;


void formula::initializeMaps()
{
    // nur einmal initialize
    if (!_symbolMap.empty())
        return;

    // mapping erstellen von ASCII zu Unicode aus symbol_mappings
    for (const auto &mapping: _symbol_mappings)
    {
        for (const char *shortcut: mapping.shortcuts)
        {
            _symbolMap[shortcut] = mapping.unicode;
        }
    }

    // ASCII zu TokenType mapping
    _operatorMap["and"] = TOK_AND;
    _operatorMap["&"] = TOK_AND;
    _operatorMap["&&"] = TOK_AND;
    _operatorMap["land"] = TOK_AND;

    _operatorMap["or"] = TOK_OR;
    _operatorMap["|"] = TOK_OR;
    _operatorMap["||"] = TOK_OR;
    _operatorMap["lor"] = TOK_OR;

    _operatorMap["not"] = TOK_NOT;
    _operatorMap["!"] = TOK_NOT;
    _operatorMap["~"] = TOK_NOT;
    _operatorMap["neg"] = TOK_NOT;

    _operatorMap["implies"] = TOK_IMP;
    _operatorMap["imp"] = TOK_IMP;
    _operatorMap["->"] = TOK_IMP;
    _operatorMap["rightarrow"] = TOK_IMP;
    _operatorMap[">"] = TOK_IMP;

    _operatorMap["iff"] = TOK_EQ;
    _operatorMap["<->"] = TOK_EQ;
    _operatorMap["<=>"] = TOK_EQ;
    _operatorMap["leftrightarrow"] = TOK_EQ;
    _operatorMap["eq"] = TOK_EQ;
    _operatorMap["equals"] = TOK_EQ;
    _operatorMap["="] = TOK_EQ;

    _operatorMap["xor"] = TOK_XOR;
    _operatorMap["^"] = TOK_XOR;

    _operatorMap["true"] = TOK_TRUE;
    _operatorMap["top"] = TOK_TRUE;
    _operatorMap["tau"] = TOK_TRUE;
    _operatorMap["tautology"] = TOK_TRUE;

    _operatorMap["false"] = TOK_FALSE;
    _operatorMap["bottom"] = TOK_FALSE;
    _operatorMap["bot"] = TOK_FALSE;
    _operatorMap["con"] = TOK_FALSE;
    _operatorMap["contradiction"] = TOK_FALSE;
}

bool formula::isBreakCharacter(const char c)
{
    return std::isspace(c) || c == '(' || c == ')' || c == '&' || c == '|' ||
           c == '!' || c == '~' || c == '>' || c == '=' || c == '^' ||
           c == '<' || c == '-';
}

//TODO if-chaos besser machen
void formula::tokenize()
{
    const std::string input(_original);
    std::string current_token;

    for (size_t i = 0; i < input.length(); ++i)
    {
        const char c = input[i];

        if (std::isspace(c))
        {
            // akkumulierte tokens
            if (!current_token.empty())
            {
                // ist es ein operator?
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    // dann ist es eine variable
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            continue;
        }

        if (c == '(')
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            _tokens.emplace_back("(", TOK_PL);
            continue;
        }

        if (c == ')')
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            _tokens.emplace_back(")", TOK_PR);
            continue;
        }

        // bissl doof, geht besser aber multi-character operators ->, <->, etc.
        if (c == '-' && i + 1 < input.length() && input[i + 1] == '>')
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            _tokens.emplace_back("->", TOK_IMP);
            i++; // skip '>'
            continue;
        }

        if (c == '<' && i + 2 < input.length() && input.substr(i, 3) == "<->")
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            _tokens.emplace_back("<->", TOK_EQ);
            i += 2; // skip "->"
            continue;
        }

        if (c == '<' && i + 2 < input.length() && input.substr(i, 3) == "<=>")
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }
            _tokens.emplace_back("<=>", TOK_EQ);
            i += 2; // skip "=>"
            continue;
        }

        // single character operators
        if (isBreakCharacter(c) && c != '-' && c != '<')
        {
            if (!current_token.empty())
            {
                auto op_it = _operatorMap.find(current_token);
                if (op_it != _operatorMap.end())
                {
                    _tokens.emplace_back(current_token, op_it->second);
                } else
                {
                    _tokens.emplace_back(current_token, TOK_VAR);
                }
                current_token.clear();
            }

            std::string single_char(1, c);
            auto op_it = _operatorMap.find(single_char);
            if (op_it != _operatorMap.end())
            {
                _tokens.emplace_back(single_char, op_it->second);
            }
            continue;
        }

        // normale character einfach dazu
        current_token += c;
    }

    // letztes token
    if (!current_token.empty())
    {
        auto op_it = _operatorMap.find(current_token);
        if (op_it != _operatorMap.end())
        {
            _tokens.emplace_back(current_token, op_it->second);
        } else
        {
            _tokens.emplace_back(current_token, TOK_VAR);
        }
    }
}

//syntactical analysis
void formula::validateTokens()
{
    if (_tokens.empty())
    {
        _invalid_reason = "Invalid formula: No Tokens!";
        return;
    }
    int parentheses_count = 0;
    bool expectVar = true;

    int current_paren_id = 0;
    std::stack<int> parentheses_stack;


    std::set<std::string> signature;

    for (size_t i = 0; i < _tokens.size(); i++)
    {
        token token = _tokens[i];
        if (expectVar)
        {
            if (!(token.isVar() || token.type == TOK_NOT || token.type == TOK_PL))
            {
                _invalid_reason = "Invalid formula: Invalid token '" + token.label + "' at position " +
                                  std::to_string(i + 1);
                return;
            }
            if (token.type == TOK_VAR)
                signature.emplace(token.label);
            if (token.isVar())
            {
                expectVar = false;
            }
            if (token.type == TOK_PL)
            {
                parentheses_count++;
                _maxdepth = std::max(parentheses_count, _maxdepth);
                if (token.label == "(")
                {
                    current_paren_id = ++_maxParenthesesID;
                    _tokens[i] = formula::token(std::to_string(current_paren_id), TOK_PL);
                    parentheses_stack.push(current_paren_id);
                }
            }
        } else
        {
            if (token.isVar() || token.type == TOK_NOT || token.type == TOK_PL)
            {
                _invalid_reason = "Invalid formula: Invalid token '" + token.label + "' at position " +
                                  std::to_string(i + 1);
                return;
            }
            if (token.type == TOK_PR)
            {
                parentheses_count--;
                if (parentheses_count < 0)
                {
                    _invalid_reason = "Invalid formula: Negative parentheses count at position " +
                                      std::to_string(i + 1);
                    return;
                }
                if (token.label == ")")
                {
                    _tokens[i] = formula::token(std::to_string(parentheses_stack.top()), TOK_PR);
                    parentheses_stack.pop();
                }
            }
            if (token.type == TOK_AND || token.type == TOK_OR || token.type == TOK_XOR || token.type == TOK_EQ || token.
                type == TOK_IMP)
                expectVar = true;
        }
    }
    _valid = parentheses_count == 0 && !expectVar;

    if (_valid)
        _invalid_reason = "Valid formula";
    else
        _invalid_reason = "Invalid formula: expected variable or ) as last token!";

    _signature = std::vector(signature.begin(), signature.end());
}

std::string formula::reconstructFromTokens() const
{
    std::ostringstream result;
    for (size_t i = 0; i < _tokens.size(); ++i)
    {
        if (i > 0)
            result << " ";
        result << _tokens[i].str();
    }
    return result.str();
}

std::string formula::getUnicodeRepresentation() const
{
    return toUnicode(reconstructFromTokens());
}

std::string formula::toUnicode(const std::string &input)
{
    if (_symbolMap.empty())
        formula temp("");

    std::string result = input;

    // Sortieren nach length (longer first) um partial replacements zu verhindern
    std::vector<std::pair<std::string, std::string> > sorted_symbols;
    for (const auto &pair: _symbolMap)
    {
        sorted_symbols.emplace_back(pair);
    }

    std::sort(sorted_symbols.begin(), sorted_symbols.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first.length() > b.first.length();
              });

    for (const auto &symbol_pair: sorted_symbols)
    {
        const std::string &ascii_symbol = symbol_pair.first;
        const std::string &unicode_symbol = symbol_pair.second;

        size_t pos = 0;
        while ((pos = result.find(ascii_symbol, pos)) != std::string::npos)
        {
            // schauen ob es ein ganzes wort ist für alphabetische operatoren
            if (ascii_symbol.length() > 1 && std::isalpha(ascii_symbol[0]))
            {
                bool is_whole_word = true;

                // char before
                if (pos > 0 && std::isalnum(result[pos - 1]))
                {
                    is_whole_word = false;
                }

                // char after
                if (pos + ascii_symbol.length() < result.length() &&
                    std::isalnum(result[pos + ascii_symbol.length()]))
                {
                    is_whole_word = false;
                }

                if (!is_whole_word)
                {
                    pos += ascii_symbol.length();
                    continue;
                }
            }

            result.replace(pos, ascii_symbol.length(), unicode_symbol);
            pos += unicode_symbol.length();
        }
    }

    return result;
}

size_t formula::findMainOp(std::vector<token> &tokens, const size_t start, const size_t end)
{
    if (tokens[start].type == TOK_PL && tokens[end].type == TOK_PR && tokens[start].label == tokens[end].label)
        return findMainOp(tokens, start + 1, end - 1);

    int bracket_depth = 0;
    size_t main_op = -1;
    int lowest_precedence = INT_MAX;

    for (size_t i = end; i >= start; i--)
    {
        if (i > end || i < start)
            break;
        if (tokens[i].type == TOK_PL)
            bracket_depth++;
        if (tokens[i].type == TOK_PR)
            bracket_depth--;

        if (bracket_depth == 0 && tokens[i].isOperator())
        {
            if (const int prec = tokens[i].getPrecedence(); prec <= lowest_precedence)
            {
                lowest_precedence = prec;
                main_op = i;
            }
        }
    }

    return main_op;
}

std::list<formula::token>::iterator formula::findMainOp(const std::list<token> &tokens,
                                                        const std::list<token>::iterator &start,
                                                        const std::list<token>::iterator &end)
{
    if (start->type == TOK_PL && end->type == TOK_PR && start->label == end->label)
        return findMainOp(tokens, std::next(start), std::prev(end));

    int bracket_depth = 0;
    std::list<token>::iterator main_op = end;
    int lowest_precedence = INT_MAX;
    bool stop = false;
    std::list<token>::iterator it = end;
    while (!stop)
    {
        if (it->type == TOK_PL)
            bracket_depth++;
        if (it->type == TOK_PR)
            bracket_depth--;

        if (bracket_depth == 0 && it->isOperator())
        {
            if (const int prec = it->getPrecedence(); prec <= lowest_precedence)
            {
                lowest_precedence = prec;
                main_op = it;
            }
        }
        stop = it == start;
        std::advance(it, -1);
    }
    return main_op;
}

void formula::evaluate()
{
    evaluate(std::vector<bool>(_signature.size(), false), std::vector<bool>(_signature.size(), true));
}

//evaluation of range, starting from start through end
void formula::evaluate(const std::vector<bool> &start, const std::vector<bool> &end)
{
    std::vector current(start);
    //ensure end>=start
    if (end != start)
        for (size_t i = start.size() - 1; i > 0; i--)
        {
            //if 1 is found in end before it is found in start, end is greater and we can stop searching
            if (end[i] && !start[i])
                break;
            //if 1 is found in start before it is found in end, start is greater and we abort
            if (start[i] && !end[i])
                return;
        }
    //bool is necessary, so the std::vector<bool>(_vars.size(), true) is included
    //(e.g., the world where every variable is assigned 1)
    bool done = false;
    while (!done)
    {
        done = current == end;
        evaluate(current);
        //add 1 to current through bitwise XOR
        for (size_t i = 0; i < current.size(); i++) // NOLINT(*-loop-convert)
        {
            current[i] = !current[i];
            if (current[i] == 1)
                break;
        }
    }
}

bool formula::evaluate(const std::vector<bool> &value)
{
    if (_evaluation_results.contains(value))
        return _evaluation_results[value];
    const bool val = evaluateRec(value, 0, _tokens.size() - 1);
    _evaluation_results[value] = val;
    return val;
}

//Returns the truth value for the given assignment, which has to be a vector<bool>, such that _vars.size() equals assignment.size();
//beside operators and parantheses, only TOK_TRUE and TOK_FALSE can be in the given formula.
//initial call with start=0 and end=tokens.size()
bool formula::evaluateRec(const std::vector<bool> &assignment, const size_t start, const size_t end)
{
    // variable found:
    if (start == end)
    {
        //return the value in the position of assignment that matches the position of the variables' label in _vars
        if (_tokens[start].type == TOK_VAR)
            return assignment[distance(_signature.begin(), std::ranges::find(_signature, _tokens[start].label))];

        if (_tokens[start].type == TOK_TRUE)
            return true;
        if (_tokens[start].type == TOK_FALSE)
            return false;
        throw std::runtime_error("Tried to evaluate non-variable token");
    }

    //for cases like ((a and b) or (b and c)), so just (a and b) or (b and c) is evaluated instead
    //label comparision, so the formula isnt turned into a and b) or ( b and c
    if (_tokens[start].type == TOK_PL && _tokens[end].type == TOK_PR && _tokens[start].label == _tokens[end].label)
        return evaluateRec(assignment, start + 1, end - 1);

    // hauptoperator finden
    int bracket_depth = 0;
    size_t main_op = -1;
    int lowest_precedence = INT_MAX;

    // von rechts nach links für richtige assozia(l)tivität
    for (size_t i = end; i >= start; i--)
    {
        if (i > end || i < start)
            break;
        if (_tokens[i].type == TOK_PL)
            bracket_depth++;
        if (_tokens[i].type == TOK_PR)
            bracket_depth--;

        if (bracket_depth == 0 && _tokens[i].isOperator())
        {
            if (const int prec = _tokens[i].getPrecedence(); prec <= lowest_precedence)
            {
                lowest_precedence = prec;
                main_op = i;
            }
        }
    }

    switch (_tokens[main_op].type)
    {
        case TOK_AND:
            return evaluateRec(assignment, start, main_op - 1) &&
                   evaluateRec(assignment, main_op + 1, end);
        case TOK_OR:
            return evaluateRec(assignment, start, main_op - 1) ||
                   evaluateRec(assignment, main_op + 1, end);
        case TOK_NOT:
            return !evaluateRec(assignment, main_op + 1, end);
        case TOK_EQ:
            return evaluateRec(assignment, start, main_op - 1) ==
                   evaluateRec(assignment, main_op + 1, end);
        case TOK_IMP:
            return !evaluateRec(assignment, start, main_op - 1) ||
                   evaluateRec(assignment, main_op + 1, end);
        case TOK_XOR:
        {
            const bool left = evaluateRec(assignment, start, main_op - 1);
            const bool right = evaluateRec(assignment, main_op + 1, end);
            return (left && !right) || (!left && right);
        }
        case TOK_TRUE:
            return true;
        case TOK_FALSE:
            return false;
        default:
            //throw std::runtime_error("Tokentype " + tokens[main_op].type + " found while trying to evaluate operator!");
            throw std::runtime_error("Invalid Tokentype found while trying to evaluate operator!");
    }
}

std::map<std::vector<bool>, bool> formula::getFullEvaluationResults()
{
    evaluate();
    return _evaluation_results;
}

//replacement of all instances of a variable with the label target within a formula with TOK_TRUE or TOK_FALSE tokens
std::vector<formula::token> formula::substitute(const std::string &target, const bool replacement)
{
    std::vector<token> result;
    for (auto &_token: _tokens)
    {
        if (_token.label == target)
            if (replacement)
                result.emplace_back("true", TOK_TRUE);
            else
                result.emplace_back("false", TOK_FALSE);
        else
            result.emplace_back(_token);
    }
    return result;
}

formula formula::forgetClassic(const std::vector<std::string> &vars, const bool skepForget) const
{
    return formula(forgetClassic(_tokens, std::set<std::string>(vars.begin(), vars.end()), skepForget));

    // std::vector resultVector(_tokens);
    //
    // for (auto &target: vars)
    // {
    //     //if target variable is not in the formula's signature, the formula stays unchanged
    //     //->skip current variable
    //     bool var_not_in_signature = false;
    //     for (auto &var: _signature)
    //     {
    //         if (target == var)
    //             break;
    //         if (var == _signature.back())
    //             var_not_in_signature = true;
    //     }
    //     if (var_not_in_signature)
    //         continue;
    //
    //     std::vector copy(resultVector);
    //     std::vector<size_t> targetPositions;
    //     std::vector<token> temp;
    //
    //     for (size_t i = 0; i < copy.size(); i++)
    //         if (copy[i].label == target)
    //             targetPositions.emplace_back(i);
    //
    //     for (const size_t targetPosition: targetPositions)
    //         copy.at(targetPosition) = token("true", TOK_TRUE);
    //     temp.emplace_back("(", TOK_PL);
    //     temp.insert(temp.end(), copy.begin(), copy.end());
    //     temp.emplace_back(")", TOK_PR);
    //     if (skepForget)
    //         temp.emplace_back("&", TOK_AND);
    //     else
    //         temp.emplace_back("|", TOK_OR);
    //     temp.emplace_back("(", TOK_PL);
    //     for (const size_t targetPosition: targetPositions)
    //         copy.at(targetPosition) = token("false", TOK_FALSE);
    //     temp.insert(temp.end(), copy.begin(), copy.end());
    //     temp.emplace_back(")", TOK_PR);
    //     resultVector = temp;
    // }
    // return formula(resultVector);
}

std::vector<formula::token> formula::forgetClassic(const std::vector<token> &tokens, const std::set<std::string> &vars,
                                                   const bool skepForget)
{
    auto resultVector(tokens);

    std::set<std::string> signature;
    for (auto &token: tokens)
        if (token.type == TOK_VAR)
            signature.insert(token.label);

    for (auto &target: vars)
    {
        //if target variable is not in the formula's signature, the formula stays unchanged
        //->skip current variable
        if (!signature.contains(target))
            continue;

        std::vector copy(resultVector);
        std::vector<size_t> targetPositions;
        std::vector<token> temp;

        for (size_t i = 0; i < copy.size(); i++)
            if (copy[i].label == target)
                targetPositions.emplace_back(i);

        for (const size_t targetPosition: targetPositions)
            copy.at(targetPosition) = token("true", TOK_TRUE);
        temp.emplace_back("(", TOK_PL);
        temp.insert(temp.end(), copy.begin(), copy.end());
        temp.emplace_back(")", TOK_PR);
        if (skepForget)
            temp.emplace_back("and", TOK_AND);
        else
            temp.emplace_back("or", TOK_OR);
        temp.emplace_back("(", TOK_PL);
        for (const size_t targetPosition: targetPositions)
            copy.at(targetPosition) = token("false", TOK_FALSE);
        temp.insert(temp.end(), copy.begin(), copy.end());
        temp.emplace_back(")", TOK_PR);
        resultVector = temp;
    }
    return resultVector;
}

formula formula::forgetSubstitute(const std::vector<std::string> &vars, const bool replacement) const
{
    std::vector resultVector(_tokens);

    for (auto &token: resultVector)
        for (const auto &var: vars)
            if (token.type == TOK_VAR && token.label == var)
            {
                if (replacement)
                    token = formula::token("true", TOK_TRUE);
                else
                    token = formula::token("false", TOK_FALSE);
            }

    return formula(resultVector);
}

/*
 * Remove variables by assuming irrelevance
 * Replaces all variable tokens from &vars with TOK_IRR, then resolves tokens to no longer contain TOK_IRR, so returned formula is a valid formula
 * result formula is just a TOK_FALSE, if the only remaining token after forgetting is a TOK_IRR
 */
formula formula::forgetTrim(const std::vector<std::string> &vars) const
{
    std::list tokensList(_tokens.begin(), _tokens.end());
    for (token &t: tokensList)
        if (t.type == TOK_VAR)
            for (const auto &var: vars)
                if (t.label == var)
                {
                    t = token("", TOK_IRR);
                    break;
                }

    forgetTrimCleanup(tokensList, tokensList.begin(), std::prev(tokensList.end()));
    std::vector resultVector(tokensList.begin(), tokensList.end());
    if (resultVector.size() == 1 && resultVector[0].type == TOK_IRR)
        resultVector[0] = token("false", TOK_FALSE);
    return formula(resultVector);
}

void formula::forgetTrimCleanup(std::list<token> &tokens, const std::list<token>::iterator start,
                                const std::list<token>::iterator end)
{
    if (start == end)
        return;

    //cleanup parentheses chains
    if (start->type == TOK_PL && end->type == TOK_PR && start->label == end->label)
    {
        while (std::next(start)->type == TOK_PL && std::prev(end)->type == TOK_PR && std::next(start)->label ==
               std::prev(end)->label)
        {
            tokens.erase(std::next(start));
            tokens.erase(std::prev(end));
        }

        forgetTrimCleanup(tokens, std::next(start), std::prev(end));

        //check for parentheses around a single TOK_IRR (negations would have been removed in child calls)
        if (std::next(start) == std::prev(end))
        {
            tokens.erase(start);
            tokens.erase(end);
        }
        return;
    }
    const auto op = findMainOp(tokens, start, end);
    auto next = std::next(op);
    auto prev = std::prev(op);

    if (op->type == TOK_NOT)
    {
        forgetTrimCleanup(tokens, next, end);
        if (std::next(op)->type == TOK_IRR)
            tokens.erase(op);
        return;
    }
    auto Start = std::prev(start);
    auto End = std::next(end);

    forgetTrimCleanup(tokens, start, prev);
    forgetTrimCleanup(tokens, next, end);

    std::advance(End, -1);
    std::advance(Start, 1);
    prev = std::prev(op);
    next = std::next(op);

    if (prev->type == TOK_IRR)
    {
        switch (op->type)
        {
            case TOK_IMP:
            case TOK_AND:
            case TOK_OR:
                tokens.erase(prev, std::next(op));
                break;
            case TOK_XOR:
            case TOK_EQ:
                tokens.erase(op, std::next(End));
            default: ;
        }
    } else if (next->type == TOK_IRR)
    {
        switch (op->type)
        {
            case TOK_IMP:
                tokens.insert(Start, token("not", TOK_NOT));
                tokens.insert(Start, token("(", TOK_PL));
                tokens.insert(op, token(")", TOK_PR));
            case TOK_AND:
            case TOK_OR:
                tokens.erase(op, std::next(next));
                break;
            case TOK_XOR:
            case TOK_EQ:
                tokens.erase(Start, next);
            default: ;
        }
    }
}

formula formula::forgetClassicRec(const std::vector<std::string> &vars) const
{
    std::list tokensCopy(_tokens.begin(), _tokens.end());

    std::set varsInSig(vars.begin(), vars.end());
    for (auto &var: varsInSig)
    {
        bool found = false;
        for (auto &sigVar: _signature)
            if (sigVar == var)
            {
                found = true;
                break;
            }
        if (!found)
            varsInSig.erase(var);
    }

    forgetClassicRecRec(tokensCopy, tokensCopy.begin(), std::prev(tokensCopy.end()), varsInSig);

    return formula(std::vector(tokensCopy.begin(), tokensCopy.end()));
}

void formula::forgetClassicRecRec(std::list<token> &tokens, const std::list<token>::iterator start,
                                  const std::list<token>::iterator end, const std::set<std::string> &vars)
{
    if (vars.empty())
        return;
    if (start == end)
    {
        if (start->type == TOK_VAR)
        {
            for (const auto &var: vars)
                if (start->label == var)
                {
                    //replace with TRUE, since we would replace a variable with TRUE OR FALSE anyways
                    tokens.insert(tokens.erase(start), token("true", TOK_TRUE));
                    break;
                }
        }
        return;
    }

    if (start->type == TOK_PL && end->type == TOK_PR && start->label == end->label)
    {
        forgetClassicRecRec(tokens, std::next(start), std::prev(end), vars);
        return;
    }

    const auto op = findMainOp(tokens, start, end);

    if (op->type == TOK_NOT)
    {
        forgetClassicRecRec(tokens, std::next(op), end, vars);
        return;
    }

    if (op->type == TOK_OR || op->type == TOK_IMP)
    {
        forgetClassicRecRec(tokens, start, std::prev(op), vars);
        forgetClassicRecRec(tokens, std::next(op), end, vars);
        return;
    }

    // std::set<std::string> varsToRemove;
    //
    // for (auto &var: vars)
    //     for (auto &sigVar: subsignature)
    //         if (var == sigVar)
    //             varsToRemove.emplace(var);

    // if (varsToRemove.empty())
    //     return;

    //bei EQ und XOR alle zu vergessenden variablen aus subformel vergessen
    if (op->type == TOK_EQ || op->type == TOK_XOR)
    {
        std::vector temp = forgetClassic(std::vector(start, std::next(end)), vars, false);
        tokens.insert(std::next(end), temp.begin(), temp.end());
        tokens.erase(start, std::next(end));
        return;

        // std::list<token> temp1;
        // for (auto it = start; it != end; ++it)
        // {
        //     if (it->type == TOK_VAR && varsToRemove.contains(it->label))
        //         temp1.emplace_back(token("TRUE", TOK_TRUE));
        //     else
        //         temp1.push_back(*it);
        // }
        // std::list<token> temp2;
        // for (auto it = start; it != end; ++it)
        // {
        //     if (it->type == TOK_VAR && varsToRemove.contains(it->label))
        //         temp2.emplace_back(token("FALSE", TOK_FALSE));
        //     else
        //         temp2.emplace_back(*it);
        // }
        // auto inserter = std::next(end);
        // tokens.erase(start, end);
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, temp1.begin(), temp1.end());
        // tokens.insert(inserter, token(")", TOK_PR));
        // tokens.insert(inserter, token("OR", TOK_OR));
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, temp2.begin(), temp2.end());
        // tokens.insert(inserter, token(")", TOK_PR));
        // tokens.insert(inserter, token(")", TOK_PR));
    }

    //jetzt ist nur noch AND übrig, also nur aus entsprechenden signaturen vergessen;
    //für links und rechts jeweils rekursion, bei beiden das gleiche vergessen wie bei EQ und XOR

    std::set<std::string> sigL;
    for (auto it = start; it != op; std::advance(it, 1))
        if (it->type == TOK_VAR)
        {
            sigL.emplace(it->label);
        }
    std::set<std::string> sigR;
    for (auto it = op; it != std::next(end); std::advance(it, 1))
        if (it->type == TOK_VAR)
        {
            sigR.emplace(it->label);
        }

    std::set<std::string> removeVarsL;
    std::set<std::string> removeVarsR;
    std::set<std::string> removeVarsBoth;

    for (auto &var: vars)
    {
        const bool l = sigL.contains(var);
        const bool r = sigR.contains(var);
        if (l && r)
        {
            removeVarsBoth.emplace(var);
            continue;
        }
        if (l)
        {
            removeVarsL.emplace(var);
            continue;
        }
        if (r)
            removeVarsR.emplace(var);
    }

    //backup iterators in case the respective tokens are modified by partial forgetting
    auto Start = std::prev(start);
    auto End = std::next(end);

    if (!removeVarsL.empty())
        forgetClassicRecRec(tokens, start, std::prev(op), removeVarsL);
    if (!removeVarsR.empty())
        forgetClassicRecRec(tokens, std::next(op), end, removeVarsR);

    std::advance(Start, 1);
    std::advance(End, -1);

    if (!removeVarsBoth.empty())
    {
        std::vector temp = forgetClassic(std::vector(Start, std::next(End)), removeVarsBoth, false);
        tokens.insert(std::next(end), temp.begin(), temp.end());
        tokens.erase(start, std::next(end));
        return;

        // std::list<token> temp1;
        // for (auto it = start; it != end; ++it)
        // {
        //     if (it->type == TOK_VAR && varsToRemove.contains(it->label))
        //         temp1.emplace_back(token("TRUE", TOK_TRUE));
        //     else
        //         temp1.push_back(*it);
        // }
        // std::list<token> temp2;
        // for (auto it = start; it != end; ++it)
        // {
        //     if (it->type == TOK_VAR && varsToRemove.contains(it->label))
        //         temp2.emplace_back(token("FALSE", TOK_FALSE));
        //     else
        //         temp2.emplace_back(*it);
        // }
        // auto inserter = std::next(end);
        // tokens.erase(start, end);
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, temp1.begin(), temp1.end());
        // tokens.insert(inserter, token(")", TOK_PR));
        // tokens.insert(inserter, token("OR", TOK_OR));
        // tokens.insert(inserter, token("(", TOK_PL));
        // tokens.insert(inserter, temp2.begin(), temp2.end());
        // tokens.insert(inserter, token(")", TOK_PR));
        // tokens.insert(inserter, token(")", TOK_PR));
    }
}

formula formula::cleanup() const
{
    std::list tokensList(_tokens.begin(), _tokens.end());

    cleanupRec(tokensList, tokensList.begin(), std::prev(tokensList.end()));

    return formula(std::vector(tokensList.begin(), tokensList.end()));
}

/*
 * cleans up the syntax of the formula given by the token vector in the given range:
 * -removes TOK_TRUE and TOK_FALSE by resolving respective operators
 * -removes multiple chained parentheses around the same formula
 * -removes multiple chained negations down to a single or no negations, respectively
 * -removes parentheses around single tokens
 * -removes syntactically equal sides of binary operators (does NOT clean formulas like not A and A)
 *
 */

void formula::cleanupRec(std::list<token> &tokens, const std::list<formula::token>::iterator start,
                         const std::list<formula::token>::iterator end)
{
    if (start == end)
        return;

    //cleanup parentheses chains
    if (start->type == TOK_PL && end->type == TOK_PR && start->label == end->label)
    {
        while (std::next(start)->type == TOK_PL && std::prev(end)->type == TOK_PR && std::next(start)->label ==
               std::prev(end)->label)
        {
            tokens.erase(std::next(start));
            tokens.erase(std::prev(end));
        }

        cleanupRec(tokens, std::next(start), std::prev(end));

        //check for parentheses around a single literal
        if ((std::next(start)->type == TOK_NOT && std::next(start, 2) == std::prev(end)) || std::next(start) ==
            std::prev(end))
        {
            tokens.erase(start);
            tokens.erase(end);
        }
        return;
    }

    const auto op = findMainOp(tokens, start, end);
    auto next = std::next(op);
    auto prev = std::prev(op);

    //main operator is a negation
    if (op->type == TOK_NOT)
    {
        //cleanup multiple chained negations
        if (std::next(op)->type == TOK_NOT)
        {
            const auto temp = std::next(next);
            tokens.erase(op, std::next(next));
            cleanupRec(tokens, temp, end);
            return;
        }
        cleanupRec(tokens, std::next(start), end);

        //negate truth value, if there is one
        if (next->type == TOK_TRUE)
        {
            tokens.erase(next);
            tokens.insert(std::next(op), token("false", TOK_FALSE));
            tokens.erase(op);
        } else if (next->type == TOK_FALSE)
        {
            tokens.erase(next);
            tokens.insert(std::next(op), token("true", TOK_TRUE));
            tokens.erase(op);
        }
        return;
    }

    //cleanup syntactically equal subformulas on both sides of the operator
    auto LeftIterator = start;
    auto RightIterator = next;
    bool identical_subformulas = true;

    while (identical_subformulas)
    {
        identical_subformulas = LeftIterator->type == RightIterator->type && LeftIterator->
                                label == RightIterator->label;
        ++LeftIterator;
        ++RightIterator;
        if (LeftIterator == op)
            break;
    }

    //formulas on both sides of main operator are syntactically equal, cleanup accordingly
    if (identical_subformulas)
    {
        if (op->type == TOK_AND || op->type == TOK_OR)
        {
            tokens.erase(op, std::next(end));
            cleanupRec(tokens, start, prev);
            return;
        }

        if (op->type == TOK_EQ || op->type == TOK_IMP)
            tokens.insert(start, token("true", TOK_TRUE));
        if (op->type == TOK_XOR)
            tokens.insert(start, token("false", TOK_FALSE));
        tokens.erase(start, std::next(end));
        return;
    }

    //backup, in case start and end are modified by cleanup of subformulas
    auto Start = std::prev(start);
    auto End = std::next(end);

    cleanupRec(tokens, start, prev);
    cleanupRec(tokens, next, end);

    std::advance(End, -1);
    std::advance(Start, 1);
    next = std::next(op);
    prev = std::prev(op);

    switch (op->type)
    {
        case TOK_AND:
            //remove left side
            if (prev->type == TOK_TRUE || next->type == TOK_FALSE)
                tokens.erase(Start, next);

                //remove right side
            else if (prev->type == TOK_FALSE || next->type == TOK_TRUE)
                tokens.erase(op, std::next(End));
            return;
        case TOK_OR:
            //remove left side
            if (prev->type == TOK_FALSE || next->type == TOK_TRUE)
                tokens.erase(Start, next);

                //remove right side
            else if (prev->type == TOK_TRUE || next->type == TOK_FALSE)
                tokens.erase(op, std::next(End));
            return;
        case TOK_XOR:
            if (prev->type == TOK_TRUE)
            {
                //remove left side, negate right side
                //cleanup right side in case of double negation or parentheses around a single literal
                tokens.insert(next, token("not", TOK_NOT));
                tokens.insert(next, token("cl", TOK_PL));
                tokens.insert(std::next(End), token("cl", TOK_PR));
                cleanupRec(tokens, std::next(op), std::next(End));
                tokens.erase(Start, std::next(op));
            }
            //remove left side
            else if (prev->type == TOK_FALSE)
            {
                tokens.erase(Start, next);
            } else if (next->type == TOK_TRUE)
            {
                //remove right side, negate left side
                //cleanup left side in case of double negation or parentheses around a single literal
                tokens.insert(Start, token("not", TOK_NOT));
                tokens.insert(Start, token("cl", TOK_PL));
                tokens.insert(op, token("cl", TOK_PR));
                cleanupRec(tokens, std::prev(Start, 2), std::prev(op));
                tokens.erase(op, std::next(End));
            } else if (next->type == TOK_FALSE)
            {
                //remove right side
                tokens.erase(op, std::next(End));
            }
            return;
        case TOK_EQ:
            if (prev->type == TOK_TRUE)
            {
                //remove left side
                tokens.erase(Start, next);
            }
            if (prev->type == TOK_FALSE)
            {
                //remove left side, negate right side
                //cleanup right side in case of double negation or parentheses around a single literal
                tokens.insert(next, token("not", TOK_NOT));
                tokens.insert(next, token("cl", TOK_PL));
                tokens.insert(std::next(End), token("cl", TOK_PR));
                cleanupRec(tokens, std::next(op), std::next(End));
                tokens.erase(Start, next);
            }
            if (next->type == TOK_TRUE)
            {
                //remove right side
                tokens.erase(op, std::next(End));
            }
            if (next->type == TOK_FALSE)
            {
                //remove right side, negate left side
                //cleanup left side in case of double negation or parentheses around a single literal
                tokens.insert(Start, token("not", TOK_NOT));
                tokens.insert(Start, token("cl", TOK_PL));
                tokens.insert(op, token("cl", TOK_PR));
                cleanupRec(tokens, std::prev(Start, 2), std::prev(op));
                tokens.erase(op, std::next(End));
            }
            return;
        case TOK_IMP:
            if (prev->type == TOK_TRUE)
            {
                //remove left side
                tokens.erase(Start, next);
            }
            if (prev->type == TOK_FALSE)
            {
                //replace all with TOK_TRUE
                tokens.insert(Start, token("true", TOK_TRUE));
                tokens.erase(Start, std::next(End));
            }
            if (next->type == TOK_TRUE)
            {
                //remove left side
                tokens.erase(Start, next);
            }
            if (next->type == TOK_FALSE)
            {
                //remove right side, negate left side
                //cleanup left side in case of double negation or parentheses around a single literal
                tokens.insert(Start, token("not", TOK_NOT));
                tokens.insert(Start, token("cl", TOK_PL));
                tokens.insert(op, token("cl", TOK_PR));
                cleanupRec(tokens, std::prev(Start, 2), std::prev(op));
                tokens.erase(op, std::next(End));
            }
            return;
        default:
            return;
    }
}
