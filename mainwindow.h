#pragma once

#include "imgui.h"
#include <vector>
#include <string>

#include "formula.h"

class MainWindow
{
    public:
        MainWindow();

        ~MainWindow();

        bool init();


        void FormulaInputWindow();

        static void InputHintWindow();

        void TestWindow();

        void FormulaListWindow();

        void ForgetWindow();

        void render();

        void shutdown();

    private:
        bool _showDemoWindow = false;
        bool _showMetricsWindow = false;
        bool _showEvaluationResults = false;
        char _input_buffer[4096] = {0};
        char _forget_buffer[4096] = {0};
        char _text_buffer[8192] = {0};
        char _unicode_buffer[8192] = {0};
        char _test_unicode[1024] = {0};
        char _test_input[256] = "a and b or not c";
        int _activeFormulaIdx = -1;
        bool test = false;

        std::vector<formula> _formulas;
        std::vector<std::map<std::vector<bool>, bool> > _evaluations;

        ImFont *_math_font = nullptr;

        bool checkFileExists(const std::string &path);

        std::string findFont(const std::string &fontName);

        void EvaluationWindow();

        static std::vector<std::string> forgetVars(const char *str);
};
