#include "mainwindow.h"

#include <charconv>

#include "imgui.h"
#include "formula.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <string_view>
#include "IconsFontAwesome6.h"

#include "imgui_internal.h"

using namespace ImGui;

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() = default;

bool MainWindow::checkFileExists(const std::string &path)
{
    const std::ifstream file(path);
    return file.good();
}

std::string MainWindow::findFont(const std::string &fontName)
{
    const std::vector<std::string> possible_paths = {
        "fonts/" + fontName,
        "../fonts/" + fontName,
        "imgui/misc/fonts/" + fontName,
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

    static constexpr ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = true;
    config.RasterizerMultiply = 1.0f;
    config.MergeMode = false;

    std::cout << "Setting up fonts..." << std::endl;
    std::string primary_font = findFont("DejaVuSans.ttf");
    if (!primary_font.empty())
    {
        _math_font = io.Fonts->AddFontFromFileTTF(primary_font.c_str(), 16.0f, &config, math_ranges);
        std::cout << "Loaded primary font: " << primary_font << std::endl;
    } else
    {
        _math_font = io.Fonts->AddFontDefault();
        std::cout << "Using default ImGui font" << std::endl;
    }

    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f; // Make icons monospaced

    std::string icon_font = findFont("Font Awesome 6 Free-Solid-900.otf");
    if (!icon_font.empty())
    {
        ImFont *icon_result = io.Fonts->AddFontFromFileTTF(
            icon_font.c_str(),
            13.0f,
            &config,
            icon_ranges
        );

        if (icon_result)
        {
            std::cout << "Successfully merged Font Awesome icons" << std::endl;
        } else
        {
            std::cout << "Failed to merge Font Awesome icons" << std::endl;
        }
    } else
    {
        std::cout << "Font Awesome not found - icons will not be available" << std::endl;
    }

    config.MergeMode = true;
    std::string math_font = findFont("STIXTwoMath-Regular.otf");
    if (!math_font.empty())
    {
        io.Fonts->AddFontFromFileTTF(math_font.c_str(), 16.0f, &config, math_ranges);
        std::cout << "Merged math font: " << math_font << std::endl;
    }

    // Build font atlas
    if (!io.Fonts->Build())
    {
        std::cout << "Font atlas build failed" << std::endl;
        return false;
    }

    std::cout << "Font atlas built successfully" << std::endl;
    std::cout << "Total fonts loaded: " << io.Fonts->Fonts.Size << std::endl;

    formula::initializeMaps();

    return true;
}

static bool show_hint = false;

void MainWindow::render()
{
    if (!_windows_initialized)
    {
        SetNextWindowPos(ImVec2(9, 10));
        SetNextWindowSize(ImVec2(343, 294));
    }
    FormulaInputWindow();
    if (show_hint)
        InputHintWindow();
    if (!_windows_initialized)
    {
        SetNextWindowPos(ImVec2(356, 8));
        SetNextWindowSize(ImVec2(914, 296));
    }
    EvaluationWindow();
    if (!_windows_initialized)
    {
        SetNextWindowPos(ImVec2(582, 312));
        SetNextWindowSize(ImVec2(687, 479));
    }
    FormulaListWindow();
    if (!_windows_initialized)
    {
        SetNextWindowPos(ImVec2(10, 313));
        SetNextWindowSize(ImVec2(569, 477));
    }
    ForgetWindow();
    _windows_initialized = true;
}

void MainWindow::FormulaInputWindow()
{
    const auto *activeFormula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : nullptr;


    if (!Begin("Formula Input & Analysis"))
    {
        End();
        return;
    }

    if (BeginChild("InputSection", ImVec2(0, 150), true))
    {
        Text("Input Formula:");
        SameLine();
        auto cur = GetCursorScreenPos();
        SetCursorScreenPos({cur.x + GetContentRegionAvail().x - CalcTextSize("W").x, cur.y});
        if (Selectable(ICON_FA_CIRCLE_INFO))
            show_hint ^= true;

        InputTextMultiline("##input", _input_buffer, sizeof(_input_buffer), ImVec2(-1, 80));

        if (Button(ICON_FA_CHECK " Parse Formula",
                   ImVec2(GetContentRegionAvail().x - CalcTextSize("W Clear").x - GetStyle().ItemSpacing.x * 2, 0)) || (
                IsItemFocused() && IsKeyPressed(ImGuiKey_Enter)) || _force_parse)
        {
            _showEvaluationResults = false;
            if (strlen(_input_buffer) > 0)
            {
                try
                {
                    if (!_force_parse)
                    {
                        _formulas.emplace_back(_input_buffer);
                        _activeFormulaIdx = _formulas.size() - 1;
                        activeFormula = &_formulas[_activeFormulaIdx];
                        if (_evaluations.capacity() < _formulas.size())
                            _evaluations.resize(_formulas.size());
                    }
                    _force_parse = false;
                    if (activeFormula->isValid())
                    {
                        printf("Formula valid");
                        strncpy(_text_buffer, "Valid formula!", sizeof(_text_buffer));
                        strncpy(_unicode_buffer, activeFormula->getUnicodeRepresentation().c_str(),
                                sizeof(_unicode_buffer));
			_text_buffer[sizeof(_text_buffer) - 1] = '\0';
			_unicode_buffer[sizeof(_unicode_buffer) - 1] = '\0';
                    } else
                    {
                        printf("Formula invalid");
                        strncpy(_text_buffer, activeFormula->getInvalidReason().c_str(), sizeof(_text_buffer));
                        strncpy(_unicode_buffer, activeFormula->getUnicodeRepresentation().c_str(),
                                sizeof(_unicode_buffer));
			_text_buffer[sizeof(_text_buffer) - 1] = '\0';
			_unicode_buffer[sizeof(_unicode_buffer) - 1] = '\0';
                    }
                } catch (const std::exception &e)
                {
                    snprintf(_text_buffer, sizeof(_text_buffer), "Error: %s", e.what());
		    _text_buffer[sizeof(_text_buffer) - 1] = '\0';
		    _unicode_buffer[0] = '\0';
                }
            }
        }
        SameLine();
        if (Button(ICON_FA_TRASH " Clear"))
        {
            _input_buffer[0] = '\0';
            _text_buffer[0] = '\0';
            _unicode_buffer[0] = '\0';
        }
    }
    EndChild();

    if (BeginChild("AnalysisSection", ImVec2(0, 0), false))
    {
        const bool has_formula = strlen(_text_buffer) > 0 && activeFormula;
        if (has_formula)
        {
            if (activeFormula->isValid())
                TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ICON_FA_CHECK " %s", _text_buffer);
            else
                TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_XMARK " %s", _text_buffer);
        }

        if (strlen(_unicode_buffer) > 0)
        {
            Text("Unicode Representation:");
            if (_math_font)
                PushFont(_math_font);

            const std::string unicode_str(_unicode_buffer);
            TextWrapped("%s", unicode_str.c_str());

            if (_math_font)
                PopFont();
        }

        if (has_formula)
        {
            if (const auto vars = activeFormula->getSignature(); !vars.empty())
            {
                Separator();
                Text("Variables (%zu):", vars.size());
                SameLine();

                PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                for (size_t i = 0; i < vars.size(); ++i)
                {
                    if (i > 0)
                    {
                        SameLine();
                        Text(",");
                    }
                    SameLine();
                    Text("%s", vars[i].c_str());
                }
                PopStyleColor();
            }
        }
    }
    EndChild();

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
    Separator();
    auto cur = GetCursorScreenPos();
    auto center = GetContentRegionAvail().x * 0.5f;
    auto btn_wdt = CalcTextSize("Close").x + GetStyle().ItemSpacing.x;
    ImVec2 pos(cur.x + center - btn_wdt * 0.5f, cur.y);
    SetCursorScreenPos(pos);
    if (Button("Close"))
        show_hint = false;
    End();
}

void MainWindow::TestWindow()
{
    if (Begin("Tests"))
    {
        if (Button("test!"))
            test = !test;
        if (test)
        {
            Text("no tests here");
        }
    }
    End();
}

void MainWindow::FormulaListWindow()
{
    if (!Begin("Formula List"))
    {
        End();
        return;
    }

    if (_formulas.empty())
    {
        TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ICON_FA_INBOX " No formulas yet");
        Text("Add one with the text input!");
        End();
        return;
    }

    if (Button(ICON_FA_TRASH " Clear All"))
    {
        _formulas.clear();
        _evaluations.clear();
        _activeFormulaIdx = -1;
    }
    SameLine();
    Text("(%zu formulas)", _formulas.size());

    Separator();

    std::vector<size_t> to_delete;

    if (BeginTable("FormulaTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
    {
        TableSetupColumn("Formula", ImGuiTableColumnFlags_WidthStretch);
        TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 40);

        for (size_t i = 0; i < _formulas.size(); i++)
        {
            const auto &formula = _formulas[i];
            bool is_selected = (i == static_cast<size_t>(_activeFormulaIdx));

            PushID(i);

            TableNextRow();
            TableNextColumn();

            char label[1024];
            if (_math_font)
                PushFont(_math_font);
            snprintf(label, sizeof(label), "[%zu] %s", i, formula.getUnicodeRepresentation().c_str());
            if (_math_font)
                PopFont();

            if (Selectable(label, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
            {
                _activeFormulaIdx = i;
                if (_formulas.capacity() > _evaluations.size())
                    _evaluations.resize(_formulas.capacity());
                if (_evaluations[_activeFormulaIdx].empty())
                    _showEvaluationResults = false;

                auto og = _formulas[_activeFormulaIdx].getOriginal();
                strncpy(_input_buffer, og.c_str(), std::min(sizeof(_input_buffer) - 1, og.size()));
                _input_buffer[og.size()] = '\0';
                _force_parse = true;
            }

            SetCursorPosX(GetCursorPosX() + 10);
            if (formula.isValid())
            {
                TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ICON_FA_CHECK);
                SameLine();
                TextDisabled("Valid | Vars: %zu", formula.getSignature().size());
            } else
            {
                TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_XMARK);
                SameLine();
                TextDisabled("Invalid");
            }

            TableNextColumn();

            if (SmallButton(ICON_FA_TRASH))
            {
                to_delete.push_back(i);
            }

            PopID();
        }

        EndTable();
    }

    for (auto it = to_delete.rbegin(); it != to_delete.rend(); ++it)
    {
        size_t idx = *it;
        _formulas.erase(_formulas.begin() + idx);

        if (_activeFormulaIdx == static_cast<int>(idx))
            _activeFormulaIdx = -1;
        else if (_activeFormulaIdx > static_cast<int>(idx))
            _activeFormulaIdx--;
    }

    End();
}

void MainWindow::ForgetWindow()
{
    if (!Begin("Forget operations and cleanup"))
    {
        End();
        return;
    }

    auto *activeFormula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : 0;

    if (activeFormula && activeFormula->isValid())
    {
        auto vars = activeFormula->getSignature();
        if (!vars.empty())
        {
            Text("Available variables:");
            SameLine();
            PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.6f, 1.0f));
            for (size_t i = 0; i < vars.size(); i++)
            {
                if (i > 0)
                {
                    SameLine();
                    Text(",");
                    SameLine();
                }
                if (SmallButton(vars[i].c_str()))
                {
                    if (strlen(_forget_buffer) > 0)
                        strcat(_forget_buffer, ", ");
                    strcat(_forget_buffer, vars[i].c_str());
                }
            }
            PopStyleColor();
            Separator();
        }
    }

    Text("Variables to forget:");
    SetNextItemWidth(-1);
    InputTextMultiline("##forget", _forget_buffer, sizeof(_forget_buffer), ImVec2(-1, 60));

    if (!activeFormula || !activeFormula->isValid())
    {
        Spacing();
        TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION" Select a valid formula first");
        End();
        return;
    }

    const std::vector<std::string> vars = forgetVars(_forget_buffer);

    Separator();

    static int selected_operation = 0;
    const char *operations[] = {
        "Cleanup Formula",
        "Classic Forget",
        "Skeptical Forget",
        "Recursive Classic Forget",
        "Replace with True",
        "Replace with False",
        "Local Irrelevance Assumption"
    };

    Text("Select operation:");
    SetNextItemWidth(GetContentRegionAvail().x - CalcTextSize("Execute Operation").x - GetStyle().ItemSpacing.x * 2);
    Combo("##operation", &selected_operation, operations, IM_ARRAYSIZE(operations));
    SameLine();
    if (Button("Execute Operation"))
    {
        switch (selected_operation)
        {
            case 0:
                _formulas.push_back(activeFormula->cleanup());
                break;
            case 1:
                _formulas.push_back(activeFormula->forgetClassic(vars, false));
                break;
            case 2:
                _formulas.push_back(activeFormula->forgetClassic(vars, true));
                break;
            case 3:
                _formulas.push_back(activeFormula->forgetClassicRec(vars));
                break;
            case 4:
                _formulas.push_back(activeFormula->forgetSubstitute(vars, true));
                break;
            case 5:
                _formulas.push_back(activeFormula->forgetSubstitute(vars, false));
                break;
            case 6:
                _formulas.push_back(activeFormula->forgetTrim(vars));
                break;
        }
    }

    End();
}

void MainWindow::EvaluationWindow()
{
    auto *formula = _activeFormulaIdx >= 0 ? &_formulas[_activeFormulaIdx] : 0;

    if (!Begin("Evaluation results - Truth Table"))
    {
        End();
        return;
    }

    if (!formula)
    {
        TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " No formula selected");
        End();
        return;
    }

    if (!formula->isValid())
    {
        TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_XMARK " Formula is not valid");
        End();
        return;
    }

    if (Button(_showEvaluationResults ? ICON_FA_EYE_SLASH " Hide Truth Table" : ICON_FA_EYE " Show Truth Table"))
        _showEvaluationResults = !_showEvaluationResults;

    if (_showEvaluationResults)
    {
        if (_formulas.capacity() > _evaluations.size())
            _evaluations.resize(_formulas.capacity());

        _evaluations[_activeFormulaIdx] = formula->getFullEvaluationResults();

        if (std::map<std::vector<bool>, bool> *eval = &_evaluations[_activeFormulaIdx]; !eval->empty())
        {
            auto it = eval->rbegin();
            const std::vector vars = formula->getSignature();

            Separator();

            int true_count = 0;
            for (const auto &[key, val]: *eval)
                if (val)
                    true_count++;

            Text("Models: %d / %d (%.1f%%)", true_count, (int) eval->size(),
                 100.0f * true_count / eval->size());

            Separator();

            if (const int columns = vars.size() + 1; BeginTable("Truth Table", columns,
                                                                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                                                ImGuiTableFlags_ScrollY))
            {
                TableSetupScrollFreeze(0, 1);
                for (size_t i = 0; i < vars.size(); i++)
                {
                    TableSetupColumn(vars[i].c_str());
                }
                TableSetupColumn("Result");
                TableHeadersRow();

                while (it != eval->rend())
                {
                    TableNextRow();
                    for (size_t i = 0; i < vars.size(); i++)
                    {
                        TableNextColumn();
                        TextColored(it->first[i] ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                                    it->first[i] ? "1" : "0");
                    }
                    TableNextColumn();
                    TextColored(it->second ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                                it->second ? "1" : "0");
                    ++it;
                }
                EndTable();
            }
        }
    }

    End();
}

std::vector<std::string> MainWindow::forgetVars(const char *str)
{
  std::vector<std::string> output;
  std::string_view input(str);
  
  size_t start = 0;
  size_t pos = 0;
  
  while (pos <= input.length())
    {
      if (pos == input.length() || input[pos] == ',')
        {
	  std::string_view token = input.substr(start, pos - start);
	  size_t first = token.find_first_not_of(" \t\n\r");
	  if (first != std::string_view::npos)
	    {
	      size_t last = token.find_last_not_of(" \t\n\r");
	      token = token.substr(first, last - first + 1);
	      output.emplace_back(token);
            }
	  start = pos + 1;
        }
        pos++;
    }
  return output;
}

void MainWindow::shutdown()
{
    //damit die IDE nicht nervt, die function static zu machen
    _activeFormulaIdx = 0;
}
