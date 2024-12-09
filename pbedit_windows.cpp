#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

class TextEditor {
public:
    TextEditor(const std::string &fileName)
        : cursorX(0), cursorY(0), offsetY(0), fileName(fileName) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        hInput = GetStdHandle(STD_INPUT_HANDLE);
        loadFile();
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        
        // Reserve space for line numbers (5 characters wide)
        lineNumberWidth = 5;
        editableWidth = consoleWidth - lineNumberWidth - 1; // -1 for spacing
        
        SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    }

    void run() {
        while (true) {
            render();
            if (!handleInput()) {
                break;
            }
        }
    }

private:
    HANDLE hConsole, hInput;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    std::string fileName;
    std::vector<std::string> lines;
    int cursorX, cursorY, offsetY;
    int consoleWidth, consoleHeight, lineNumberWidth, editableWidth;

    void loadFile() {
        std::ifstream file(fileName);
        if (file) {
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
        } else {
            lines.emplace_back(); // Start with one empty line
        }
    }

    void saveFile() {
        std::ofstream file(fileName);
        for (const auto &line : lines) {
            file << line << '\n';
        }
    }

    void render() {
        // Ensure the buffer can hold the full screen including status bar
        std::vector<CHAR_INFO> screenBuffer(consoleWidth * consoleHeight);
        SMALL_RECT writeRegion = {0, 0, static_cast<SHORT>(consoleWidth - 1), static_cast<SHORT>(consoleHeight - 1)};
        COORD bufferSize = {static_cast<SHORT>(consoleWidth), static_cast<SHORT>(consoleHeight)};
        COORD bufferCoord = {0, 0};

        // Render lines
        for (int i = 0; i < consoleHeight - 1; ++i) {
            int lineIndex = i + offsetY;
            std::string line;
            std::string lineNumberStr;

            if (lineIndex < lines.size()) {
                // Truncate line to editable width
                line = lines[lineIndex].substr(0, editableWidth);
                
                // Create line number string
                lineNumberStr = std::to_string(lineIndex + 1);
                lineNumberStr = std::string(lineNumberWidth - lineNumberStr.length(), ' ') + lineNumberStr + " ";
            }

            // Fill the buffer with the line content
            for (int j = 0; j < consoleWidth; ++j) {
                int bufferIndex = i * consoleWidth + j;
                
                if (j < lineNumberWidth) {
                    // Render line numbers
                    if (j < lineNumberStr.length()) {
                        screenBuffer[bufferIndex].Char.AsciiChar = lineNumberStr[j];
                        // Use a different color for line numbers
                        screenBuffer[bufferIndex].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE; // Cyan
                    } else {
                        screenBuffer[bufferIndex].Char.AsciiChar = ' ';
                        screenBuffer[bufferIndex].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    }
                } else if (j < lineNumberWidth + editableWidth) {
                    // Render text content
                    int textIndex = j - lineNumberWidth;
                    if (textIndex < line.length()) {
                        screenBuffer[bufferIndex].Char.AsciiChar = line[textIndex];
                        screenBuffer[bufferIndex].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White text
                    } else {
                        screenBuffer[bufferIndex].Char.AsciiChar = ' ';
                        screenBuffer[bufferIndex].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    }
                } else {
                    // Render empty space after text
                    screenBuffer[bufferIndex].Char.AsciiChar = ' ';
                    screenBuffer[bufferIndex].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                }
            }
        }

        // Render status bar
        std::string statusStr = "Ctrl+S: Save | ESC: Exit | "
                                + fileName
                                + " | Line: " + std::to_string(cursorY + 1 + offsetY)
                                + ", Col: " + std::to_string(cursorX + 1);

        // Truncate status string to console width if necessary
        statusStr = statusStr.substr(0, consoleWidth);

        // Fill status bar
        int statusBarIndex = (consoleHeight - 1) * consoleWidth;
        for (int j = 0; j < consoleWidth; ++j) {
            if (j < statusStr.length()) {
                screenBuffer[statusBarIndex + j].Char.AsciiChar = statusStr[j];
                // Inverted colors for status bar
                screenBuffer[statusBarIndex + j].Attributes = BACKGROUND_GREEN | BACKGROUND_BLUE |
                                                            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            } else {
                screenBuffer[statusBarIndex + j].Char.AsciiChar = ' ';
                // Maintain inverted colors for empty status bar space
                screenBuffer[statusBarIndex + j].Attributes = BACKGROUND_GREEN | BACKGROUND_BLUE |
                                                            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            }
        }

        // Write the entire buffer at once
        WriteConsoleOutput(hConsole, screenBuffer.data(), bufferSize, bufferCoord, &writeRegion);

        // Move cursor to current editing position
        COORD cursorPos = {static_cast<SHORT>(cursorX + lineNumberWidth + 1), static_cast<SHORT>(cursorY)};
        SetConsoleCursorPosition(hConsole, cursorPos);
    }

    bool handleInput() {
        INPUT_RECORD input;
        DWORD events;
        ReadConsoleInput(hInput, &input, 1, &events);

        if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
            int currentLineIndex = cursorY + offsetY;

            switch (input.Event.KeyEvent.wVirtualKeyCode) {
            case VK_ESCAPE:
                return false;
            case 'S': // Ctrl+S
                if (input.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) {
                    saveFile();
                }
                break;
            case VK_UP:
                if (currentLineIndex > 0) {
                    if (cursorY > 0) {
                        cursorY--;
                    } else {
                        offsetY--;
                    }
                    
                    // Ensure cursor doesn't go beyond line length
                    if (cursorX >= lines[currentLineIndex - 1].length()) {
                        cursorX = lines[currentLineIndex - 1].length();
                    }
                }
                break;
            case VK_DOWN:
                if (currentLineIndex < lines.size() - 1) {
                    if (cursorY < consoleHeight - 2) {
                        cursorY++;
                    } else {
                        offsetY++;
                    }
                    
                    // Ensure cursor doesn't go beyond line length
                    if (cursorX >= lines[currentLineIndex + 1].length()) {
                        cursorX = lines[currentLineIndex + 1].length();
                    }
                }
                break;
            case VK_LEFT:
                if (cursorX > 0) {
                    cursorX--;
                } else if (currentLineIndex > 0) {
                    // Move to previous line's end
                    cursorY--;
                    if (cursorY < 0) {
                        offsetY--;
                        cursorY = 0;
                    }
                    cursorX = lines[currentLineIndex - 1].length();
                }
                break;
            case VK_RIGHT:
                if (cursorX < lines[currentLineIndex].length()) {
                    cursorX++;
                } else if (currentLineIndex < lines.size() - 1) {
                    // Move to next line's start
                    cursorY++;
                    if (cursorY >= consoleHeight - 1) {
                        offsetY++;
                        cursorY = consoleHeight - 2;
                    }
                    cursorX = 0;
                }
                break;
            case VK_BACK:
                if (cursorX > 0) {
                    // Existing backspace behavior for non-zero cursor X
                    lines[currentLineIndex].erase(cursorX - 1, 1);
                    cursorX--;
                } else if (currentLineIndex > 0) {
                    // If at the beginning of a line and not the first line
                    std::string currentLine = lines[currentLineIndex];

                    // Remove current line and merge with previous line
                    lines.erase(lines.begin() + currentLineIndex);

                    // Move cursor to the end of the previous line
                    cursorY--;
                    if (cursorY < 0) {
                        offsetY--;
                        cursorY = 0;
                    }

                    cursorX = lines[currentLineIndex - 1].length();
                    lines[currentLineIndex - 1] += currentLine;
                }
                break;
            case VK_RETURN:
                lines.insert(lines.begin() + currentLineIndex + 1,
                            lines[currentLineIndex].substr(cursorX));
                lines[currentLineIndex] = lines[currentLineIndex].substr(0, cursorX);
                cursorY++;
                if (cursorY >= consoleHeight - 1) {
                    offsetY++;
                    cursorY = consoleHeight - 2;
                }
                cursorX = 0;
                break;
            default:
                char c = input.Event.KeyEvent.uChar.AsciiChar;
                if (c >= 32 && c <= 126) { // Printable characters
                    lines[currentLineIndex].insert(cursorX, 1, c);
                    cursorX++;
                }
                break;
            }
        }
        return true;
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: PEdit <filename>" << std::endl;
        return 1;
    }
    TextEditor editor(argv[1]);
    editor.run();
    return 0;
}
