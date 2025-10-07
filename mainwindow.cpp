#include "mainwindow.h"

#include <charconv>

#include "imgui.h"
#include "formula.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <list>

#include "imgui_internal.h"

using namespace ImGui;

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() = default;

bool MainWindow::checkFileExists(const std::string &path)
{
    std::ifstream file(path);
    return file.good();
}

std::string MainWindow::findFont(const std::string &fontName)
{
    std::vector<std::string> possible_paths = {
        "fonts/" + fontName,
        "../fonts/" + fontName,
        "../../misc/fonts/" + fontName,
        "/usr/share/fonts/truetype/dejavu/" + fontName,
        "/System/Library/Fonts/" + fontName, // macOS
        "C:/Windows/Fonts/" + fontName, // Windows
        "/usr/share/fonts/TTF/" + fontName, // Arch Linux btw
        "/usr/share/fonts/opentype/dejavu/" + fontName, //Linux distros
    };

    for (const auto &path: possible_paths)
    {
        if (checkFileExists(path))
        {
            std::cout << "Found font: " << path << std::endl;
            return path;
        }
    }

    std::cout << "Warning: Font '" << fontName << "' not found in any standard location" << std::endl;
    return "";
}

bool MainWindow::init()
{
    ImGuiIO &io = GetIO();
    static const ImWchar math_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x0370, 0x03FF, // Greek and Coptic
        0x2000, 0x206F, // General Punctuation
        0x2070, 0x209F, // Superscripts and Subscripts
        0x20A0, 0x20CF, // Currency Symbols
        0x2100, 0x214F, // Letterlike Symbols
        0x2150, 0x218F, // Number Forms
        0x2190, 0x21FF, // Arrows
        0x2200, 0x22FF, // Mathematical Operators
        0x2300, 0x23FF, // Miscellaneous Technical
        0x25A0, 0x25FF, // Geometric Shapes
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x27C0, 0x27EF, // Miscellaneous Mathematical Symbols-A
        0x2980, 0x29FF, // Miscellaneous Mathematical Symbols-B
        0x2A00, 0x2AFF, // Supplemental Mathematical Operators
        0x2B00, 0x2BFF, // Miscellaneous Symbols and Arrows
        0
    };

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = true;
    config.RasterizerMultiply = 1.0f;

    std::cout << "Setting up fonts..." << std::endl;

    std::vector<std::string> font_candidates = {
        "STIXTwoMath-Regular.otf", // mathematical font
        "NotoSansMath-Regular.ttf", // fallback
        "DejaVuSans.ttf", // system font
        "arial.ttf", // Windows fallback
        "Liberation-Sans.ttf", // Linux fallback
    };

    _math_font = nullptr;

    for (const auto &font_name: font_candidates)
    {
        std::string font_path = findFont(font_name);
        if (!font_path.empty())
        {
            _math_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, &config, math_ranges);
            if (_math_font)
            {
                std::cout << "Successfully loaded math font: " << font_path << std::endl;
                break;
            } else
            {
                std::cout << "Failed to load font: " << font_path << std::endl;
            }
        }
    }

    // Fallback to default
    if (!_math_font)
    {
        std::cout << "Using default ImGui font" << std::endl;
        _math_font = io.Fonts->AddFontDefault();
        config.MergeMode = true;
        for (const auto &font_name: font_candidates)
        {
            std::string font_path = findFont(font_name);
            if (!font_path.empty())
            {
                ImFont *merged = io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, &config, math_ranges);
                if (merged)
                {
                    std::cout << "Merged additional symbols from: " << font_path << std::endl;
                    break;
                }
            }
        }
    }

    // Build font atlas
    bool font_built = io.Fonts->Build();
    if (!font_built)
    {
        std::cout << "✗ Font atlas build failed" << std::endl;
        return false;
    }

    std::cout << "✓ Font atlas built successfully" << std::endl;
    std::cout << "Total fonts loaded: " << io.Fonts->Fonts.Size << std::endl;

    formula::initializeMaps();

    return true;
}

void MainWindow::render()
{
    FormulaInputWindow();

    InputHintWindow();

    EvaluationWindow();

    //TestWindow();

    FormulaListWindow();

    ForgetWindow();
}

void MainWindow::FormulaInputWindow()
{
    const auto *activeFormula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : 0;
    Begin("Formula Input & Analysis");

    Text("Input Formula:");
    InputTextMultiline("##input", _input_buffer, sizeof(_input_buffer),
                       ImVec2(-1, 100));

    if (Button("Parse Formula") || (IsItemFocused() && IsKeyPressed(ImGuiKey_Enter)))
    {
        _showEvaluationResults = false;
        if (strlen(_input_buffer) > 0)
        {
            try
            {
                // parse formula
                _formulas.emplace_back(_input_buffer);
                _activeFormulaIdx = _formulas.size() - 1;
                activeFormula = &_formulas[_activeFormulaIdx];
                if (_evaluations.capacity() < _formulas.size())
                    _evaluations.resize(_formulas.size());
                if (activeFormula->isValid())
                {
                    strncpy(_text_buffer, "Valid formula!", sizeof(_text_buffer));
                    strncpy(_unicode_buffer, activeFormula->getUnicodeRepresentation().c_str(),
                            sizeof(_unicode_buffer));
                } else
                {
                    strncpy(_text_buffer, activeFormula->getInvalidReason().c_str(),
                            sizeof(_text_buffer));
                    //_unicode_buffer[0] = '\0';
                    strncpy(_unicode_buffer, activeFormula->getUnicodeRepresentation().c_str(),
                            sizeof(_unicode_buffer));
                }
            } catch (const std::exception &e)
            {
                snprintf(_text_buffer, sizeof(_text_buffer), "Error: %s", e.what());
                _unicode_buffer[0] = '\0';
            }
        }
    }

    SameLine();
    if (Button("Clear"))
    {
        _input_buffer[0] = '\0';
        _text_buffer[0] = '\0';
        _unicode_buffer[0] = '\0';
    }

    Separator();

    if (strlen(_text_buffer) > 0 && activeFormula)
    {
        if (activeFormula->isValid())
        {
            TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", _text_buffer);
        } else
        {
            TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", _text_buffer);
        }
    }

    if (strlen(_unicode_buffer) > 0)
    {
        Text("Unicode Representation:");
        if (_math_font)
            PushFont(_math_font);

        const std::string unicode_str(_unicode_buffer);

        if (unicode_str.find("∧") != std::string::npos)
        {
            TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", unicode_str.c_str());
        } else
        {
            Text("%s", unicode_str.c_str());
        }

        if (_math_font)
            PopFont();
    }
    if (activeFormula)
    {
        if (const auto vars = activeFormula->getSignature(); !vars.empty())
        {
            Text("Variables found (%zu):", vars.size());
            for (size_t i = 0; i < vars.size(); ++i)
            {
                if (i > 0)
                    SameLine();
                TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", vars[i].c_str());
                if (i < vars.size() - 1)
                {
                    SameLine();
                    Text(",");
                }
            }
        }
    }
    End();
}

void MainWindow::InputHintWindow()
{
    Begin("Formula input hints");
    Text("Logical AND: 'and','land','&','&&'");
    Text("Logical OR: 'or','lor','|','||'");
    Text("Locical NOT: 'not','!','~','neg'");
    Text("Implication: 'implies', 'imp', '->', 'rightarrow', '>'");
    Text("Equivalence: 'iff', '<->', '<=>', 'leftrightarrow', 'eq', 'equals', '='");
    Text("XOR: 'xor', '^'");
    Text("logical TRUE: 'true', 'T', 'top', 'tau', 'tautology'");
    Text("logical FALSE: 'false', 'F', 'top', 'con', 'contradiction'");
    End();
}

void MainWindow::TestWindow()
{
    if (Begin("Tests"))
    {
        if (Button("test!"))
            test = true;
        if (test)
        {
            // std::vector<std::string> a;
            //
            // a.resize(5);
            // a[1] = "eins";
            // a[3] = "drei";
            // for (size_t i = 0; i < a.size(); i++)
            // {
            //     Text(a[i].c_str());
            // }
            // Separator();
            // Text("a size: %d", a.size());
            // Text("a capactiy: %d", a.capacity());
            // a.resize(20);
            // Separator();
            // a[10] = "zehn";
            // a[15] = "fünfzehn";
            // Text("nach resize");
            // for (size_t i = 0; i < a.size(); i++)
            // {
            //     Text(a[i].c_str());
            // }
            // Text("a size: %d", a.size());
            // Text("a capactiy: %d", a.capacity());

            /*
            auto testF = formula("(a or b)->c");
            std::vector<std::string> vars;
            vars.emplace_back("a");
            auto testF2 = testF.forgetTrim(vars);
            Text(testF.getUnicodeRepresentation().c_str());
            Separator();
            Text(testF2.getUnicodeRepresentation().c_str());
            */
            //Separator();
            /*
            std::list<std::string> aaaa;
            aaaa.emplace_back("1");
            aaaa.emplace_back("2");
            aaaa.emplace_back("3");
            aaaa.emplace_back("4");
            aaaa.emplace_back("5");
            std::list<std::string>::iterator two = aaaa.begin();
            std::advance(two, 1);
            std::list<std::string>::iterator four = two;
            std::advance(four, 2);
            aaaa.insert(four, "a");
            Text("two: ");
            SameLine();
            Text(two->c_str());
            Text("four: ");
            SameLine();
            Text(four->c_str());
            Text("End: ");
            SameLine();
            Text(std::prev(aaaa.end())->c_str());
            for (auto &e :aaaa)
            {
                Text(e.c_str());
            }
            */
            //Separator();
            /*
            std::list<std::string> tokens;
            tokens.emplace_back("FALSE");
            tokens.emplace_back("OR");
            tokens.emplace_back("NOT");
            tokens.emplace_back("(");
            tokens.emplace_back("a");
            tokens.emplace_back("XOR");
            tokens.emplace_back("b");
            tokens.emplace_back(")");

            auto it = tokens.begin();
            auto op = std::next(tokens.begin());
            auto end = std::prev(tokens.end());

            while (it!=tokens.end())
            {
                Text(it++ -> c_str());
                if (it!=tokens.end())SameLine();
            }
            Separator();
            tokens.insert(std::next(op), "(");
            tokens.insert(std::next(op), "NOT");
            tokens.insert(std::next(end), ")");
            tokens.erase(std::prev(op), std::next(op));

            it = tokens.begin();
            while (it!=tokens.end())
            {
                Text(it++ -> c_str());
                if (it!=tokens.end())SameLine();
            }
            */
            //Separator();
            std::list<std::string> tests;
            tests.push_back("(");
            tests.push_back("a");
            tests.push_back(")");
            auto start = tests.begin();
            std::advance(start, -2);
            Text(start->c_str());
        }
    }
    End();
}

void MainWindow::FormulaListWindow()
{
    if (Begin("Formula List"))
    {
        if (_formulas.empty())
        {
            Text("No formulas yet. Add one with the text input!");
            End();
            return;
        }

        for (size_t i = 0; i < _formulas.size(); i++)
        {
            const auto &formula = _formulas[i];
            Spacing();
            Separator();
            Spacing();
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "(%lld) %s\n%s\n\t",
                     i,
                     formula.getOriginal().c_str(),
                     formula.getUnicodeRepresentation().c_str());
            const ImVec2 cursor_pos = GetCursorScreenPos();
            if (Selectable(buffer, i == static_cast<size_t>(_activeFormulaIdx)))
            {
                _activeFormulaIdx = i;
                if (_formulas.capacity() > _evaluations.size())
                    _evaluations.resize(_formulas.capacity());
                if (_evaluations[_activeFormulaIdx].empty())
                    _showEvaluationResults = false;
                //TODO hier
                auto og = _formulas[_activeFormulaIdx].getOriginal();
                strncpy(_input_buffer, og.c_str(), size(og));
            }
            SetCursorScreenPos(ImVec2(cursor_pos.x,
                                      cursor_pos.y + GetTextLineHeight() * 2));
            if (formula.isValid())
                TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Valid Formula");
            else
                TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s",
                            formula.getInvalidReason().c_str());
        }
    }
    End();
}

void MainWindow::ForgetWindow()
{
    auto *activeFormula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : 0;

    if (Begin("Forget operations and cleanup"))
    {
        Text("Enter variables to be forgotten from selected formula (comma-seperated list):");
        InputTextMultiline("##forget", _forget_buffer, sizeof(_forget_buffer));
        if (!activeFormula)
            Text("Add a formula to use forget operations");
        else
        {
            if (Button("Cleanup Formula"))
            {
                _formulas.push_back(activeFormula->cleanup());
            }
            if (Button("Classic Forget"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetClassic(vars, false));
            }
            if (Button("Skeptical Forget"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetClassic(vars, true));
            }
            if (Button("Replace with true"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetSubstitute(vars, true));
            }
            if (Button("Replace with false"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetSubstitute(vars, false));
            }
            if (Button("Local irrelevance assumption"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetTrim(vars));
            }
            if (Button("Recursive Classic forget"))
            {
                const std::vector<std::string> vars = forgetVars(_forget_buffer);
                _formulas.push_back(activeFormula->forgetClassicRec(vars));
            }
        }
    }
    End();
}

void MainWindow::EvaluationWindow()
{
    auto *formula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : 0;
    if (Begin("Evaluation results - Truth Table"))
    {
        if (Button("Show full truth table"))
            _showEvaluationResults = true;

        if (_showEvaluationResults)
        {
            if (!formula)
            {
                Text("No Formula!");
                End();
                return;
            }

            if (!formula->isValid())
            {
                Text("No valid Formula!");
                End();
                return;
            }
            if (_formulas.capacity() > _evaluations.size())
                _evaluations.resize(_formulas.capacity());
            _evaluations[_activeFormulaIdx] = formula->getFullEvaluationResults();
            if (std::map<std::vector<bool>, bool> *eval = &_evaluations[_activeFormulaIdx]; eval->empty())
            {
                Text("No evaluation results!");
                End();
                return;
            } else
            {
                auto it = eval->rbegin();
                const std::vector vars = formula->getSignature();
                //+1 damit das ergebnis auch rein kann
                if (const int columns = vars.size() + 1; BeginTable("Truth Table", columns))
                {
                    for (size_t i = 0; i < vars.size(); i++)
                    {
                        TableNextColumn();
                        Text(vars[i].c_str());
                    }
                    TableNextColumn();
                    Text("Result");
                    while (it != eval->rend())
                    {
                        for (size_t i = 0; i < vars.size(); i++)
                        {
                            TableNextColumn();
                            if (it->first[i])
                                Text("1");
                            else
                                Text("0");
                        }
                        TableNextColumn();
                        if (it->second)
                            Text("1");
                        else
                            Text("0");
                        ++it;
                    }
                    EndTable();
                }
            }
        }
    }
    End();
}

std::vector<std::string> MainWindow::forgetVars(const char *str)
{
    //HELP, das geht bestimmt saubererer oder
    std::vector<std::string> output;
    std::string buffer;

    for (size_t i = 0; i < strlen(str); i++)
    {
        if (str[i] == ',')
        {
            output.push_back(buffer);
            buffer.clear();
        } else
        {
            buffer += str[i];
        }
    }
    if (!buffer.empty())
        output.push_back(buffer);
    return output;
}


void MainWindow::shutdown()
{
    //damit die IDE nicht nervt, die function static zu machen
    _activeFormulaIdx = 0;
}
