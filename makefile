# Cross-Platform Makefile for PLTool
# Supports: Linux, Windows (MinGW), Cross-compilation

# Detect OS
ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
	EXE_EXT := .exe
	MKDIR := mkdir
	RM := del /Q
	PATH_SEP := \\
else
	DETECTED_OS := $(shell uname -s)
	EXE_EXT :=
	MKDIR := mkdir -p
	RM := rm -f
	PATH_SEP := /
endif

# Configuration
OUT_DIR = Build
TARGET_NAME = PLTool
EXE = $(OUT_DIR)$(PATH_SEP)$(TARGET_NAME)$(EXE_EXT)
IMGUI_DIR = imgui

# Source files
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp
SOURCES += $(IMGUI_DIR)/imgui_demo.cpp
SOURCES += $(IMGUI_DIR)/imgui_draw.cpp
SOURCES += $(IMGUI_DIR)/imgui_tables.cpp
SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += formula.cpp
SOURCES += mainwindow.cpp

# Platform-specific backend
ifeq ($(DETECTED_OS),Windows)
	SOURCES += $(IMGUI_DIR)/backends/imgui_impl_win32.cpp
else
	SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
endif

# Object files
OBJS = $(addprefix $(OUT_DIR)$(PATH_SEP), $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -g -Wall -Wformat -Wextra

# Platform-specific settings
ifeq ($(DETECTED_OS),Windows)
	# Windows (MinGW)
	CXXFLAGS += -DUNICODE -static-libgcc -static-libstdc++
	LIBS = -lopengl32 -lgdi32 -ldwmapi
	LDFLAGS = -static -mwindows
else ifeq ($(DETECTED_OS),Linux)
	# Linux
	CXXFLAGS += -fPIC
	LIBS = -lGL -ldl `pkg-config --libs glfw3`
	LDFLAGS = 
else
	# Default/Other Unix
	LIBS = -lGL -ldl
	LDFLAGS = 
endif

# Include directories
CXXFLAGS += -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends

# Font directories (try multiple common locations)
FONT_PATHS = -DFONT_PATH_1=\"fonts/\" \
             -DFONT_PATH_2=\"../fonts/\" \
             -DFONT_PATH_3=\"../../misc/fonts/\" \
             -DFONT_PATH_4=\"/usr/share/fonts/truetype/dejavu/\" \
             -DFONT_PATH_5=\"C:/Windows/Fonts/\"

CXXFLAGS += $(FONT_PATHS)

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

.PHONY: all clean run install help linux windows simple mingw test-mingw

all: $(EXE)
	@echo "Build complete for $(DETECTED_OS)"
	@echo "Executable: $(EXE)"

$(EXE): $(OBJS) | $(OUT_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "Linking complete: $@"

# Create output directory
$(OUT_DIR):
ifeq ($(DETECTED_OS),Windows)
	if not exist $(OUT_DIR) $(MKDIR) $(OUT_DIR)
else
	$(MKDIR) $(OUT_DIR)
endif

# Compile main.cpp
$(OUT_DIR)$(PATH_SEP)main.o: main.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile ImGui core files
$(OUT_DIR)$(PATH_SEP)imgui.o: $(IMGUI_DIR)/imgui.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_DIR)$(PATH_SEP)imgui_demo.o: $(IMGUI_DIR)/imgui_demo.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_DIR)$(PATH_SEP)imgui_draw.o: $(IMGUI_DIR)/imgui_draw.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_DIR)$(PATH_SEP)imgui_tables.o: $(IMGUI_DIR)/imgui_tables.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_DIR)$(PATH_SEP)imgui_widgets.o: $(IMGUI_DIR)/imgui_widgets.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile backend files (platform-specific)
$(OUT_DIR)$(PATH_SEP)imgui_impl_opengl3.o: $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ifeq ($(DETECTED_OS),Windows)
$(OUT_DIR)$(PATH_SEP)imgui_impl_win32.o: $(IMGUI_DIR)/backends/imgui_impl_win32.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
$(OUT_DIR)$(PATH_SEP)imgui_impl_glfw.o: $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

# Compile project files
$(OUT_DIR)$(PATH_SEP)mainwindow.o: mainwindow.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_DIR)$(PATH_SEP)formula.o: formula.cpp | $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Platform-specific targets
linux: 
	@echo "Building for Linux with GLFW..."
	$(MAKE) all DETECTED_OS=Linux

windows:
	@echo "Building for Windows with Win32..."
	$(MAKE) all DETECTED_OS=Windows

# Cross-compile for Windows using MinGW on Linux  
mingw: | $(OUT_DIR)
	@echo "Cross-compiling for Windows using MinGW..."
	@echo "Building native Windows executable..."
	@echo "Step 1/4: Compiling project files..."
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -DUNICODE -DWIN32 -D_WIN32 -static-libgcc -static-libstdc++ -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends $(FONT_PATHS) -c main.cpp -o Build/main_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -DUNICODE -DWIN32 -D_WIN32 -static-libgcc -static-libstdc++ -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends $(FONT_PATHS) -c formula.cpp -o Build/formula_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -DUNICODE -DWIN32 -D_WIN32 -static-libgcc -static-libstdc++ -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends $(FONT_PATHS) -c mainwindow.cpp -o Build/mainwindow_mingw.o
	@echo "Step 2/4: Compiling ImGui core..."
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -c $(IMGUI_DIR)/imgui.cpp -o Build/imgui_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -c $(IMGUI_DIR)/imgui_demo.cpp -o Build/imgui_demo_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -c $(IMGUI_DIR)/imgui_draw.cpp -o Build/imgui_draw_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -c $(IMGUI_DIR)/imgui_tables.cpp -o Build/imgui_tables_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -c $(IMGUI_DIR)/imgui_widgets.cpp -o Build/imgui_widgets_mingw.o
	@echo "Step 3/4: Compiling ImGui backends..."
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -c $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp -o Build/imgui_impl_opengl3_mingw.o
	x86_64-w64-mingw32-g++ -std=c++20 -O2 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -c $(IMGUI_DIR)/backends/imgui_impl_win32.cpp -o Build/imgui_impl_win32_mingw.o
	@echo "Step 4/4: Linking Windows executable..."
	x86_64-w64-mingw32-g++ -static -mwindows -static-libgcc -static-libstdc++ \
		Build/main_mingw.o Build/formula_mingw.o Build/mainwindow_mingw.o \
		Build/imgui_mingw.o Build/imgui_demo_mingw.o Build/imgui_draw_mingw.o \
		Build/imgui_tables_mingw.o Build/imgui_widgets_mingw.o \
		Build/imgui_impl_opengl3_mingw.o Build/imgui_impl_win32_mingw.o \
		-lopengl32 -lgdi32 -ldwmapi -luser32 -lkernel32 -lshell32 \
		-o Build/PLTool.exe
	@echo "✓ Windows executable created: Build/PLTool.exe"
	@echo "  Size: $(ls -lh Build/PLTool.exe | awk '{print $5}')"
	@echo "  You can copy this to a Windows machine to run it."

# Simple build (recommended for Linux)
simple:
	@echo "Building simple Linux version..."
	$(MAKE) all DETECTED_OS=Linux

# Test MinGW installation
test-mingw:
	@echo "Testing MinGW installation..."
	@which x86_64-w64-mingw32-g++ > /dev/null && echo "✓ MinGW compiler found" || echo "✗ MinGW compiler not found"
	@x86_64-w64-mingw32-g++ --version 2>/dev/null | head -1 || echo "✗ MinGW not working"
	@echo "If MinGW is missing, run: make install-deps-mingw"

# Run the application
run: $(EXE)
ifeq ($(DETECTED_OS),Windows)
	$(EXE)
else
	./$(EXE)
endif

# Install dependencies (Linux)
install-deps-linux:
	@echo "Installing dependencies for Linux..."
	sudo apt-get update
	sudo apt-get install -y build-essential libglfw3-dev libgl1-mesa-dev pkg-config
	sudo apt-get install -y fonts-dejavu-core fonts-dejavu-extra

# Install dependencies (Windows/MinGW)
install-deps-windows:
	@echo "Installing dependencies for Windows..."
	@echo "Please install MinGW-w64 and ensure it's in your PATH"
	@echo "Download from: https://www.mingw-w64.org/downloads/"

# Install MinGW cross-compiler on Linux
install-deps-mingw:
	@echo "Installing MinGW cross-compiler for Linux..."
	sudo apt-get update
	sudo apt-get install -y mingw-w64 gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
	@echo "MinGW installation complete"
	@echo "You can now use: make mingw"

# Setup fonts
setup-fonts:
	@echo "Setting up fonts..."
ifeq ($(DETECTED_OS),Windows)
	if not exist fonts $(MKDIR) fonts
	copy "C:\Windows\Fonts\arial.ttf" fonts\ 2>nul || echo "Arial not found"
	copy "C:\Windows\Fonts\DejaVuSans.ttf" fonts\ 2>nul || echo "DejaVu not found"
else
	$(MKDIR) fonts 2>/dev/null || true
	cp /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf fonts/ 2>/dev/null || echo "DejaVu not found in /usr/share/fonts"
	cp /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf fonts/ 2>/dev/null || true
	cp /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf fonts/ 2>/dev/null || true
endif
	@echo "Fonts setup complete"

# Clean build files
clean:
ifeq ($(DETECTED_OS),Windows)
	if exist $(OUT_DIR) rmdir /S /Q $(OUT_DIR)
else
	$(RM) -rf $(OUT_DIR)
endif
	@echo "Clean complete"

# Help
help:
	@echo "Available targets:"
	@echo "  all              - Build for current platform"
	@echo "  linux            - Build for Linux (GLFW)"
	@echo "  simple           - Simple Linux build (recommended)"
	@echo "  windows          - Build for Windows (Win32)"
	@echo "  mingw            - Cross-compile for Windows using MinGW"
	@echo "  run              - Build and run the application"
	@echo "  clean            - Remove build files"
	@echo "  setup-fonts      - Copy system fonts to local directory"
	@echo "  install-deps-*   - Install platform dependencies"
	@echo "  test-mingw       - Test MinGW cross-compiler installation"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Current platform: $(DETECTED_OS)"
	@echo "Target executable: $(EXE)"

# Debug info
debug-info:
	@echo "=== Build Configuration ==="
	@echo "OS: $(DETECTED_OS)"
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJS: $(OBJS)"
	@echo "EXE: $(EXE)"
