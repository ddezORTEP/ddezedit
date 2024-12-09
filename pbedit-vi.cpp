// -------------------------------------------
// |     PBedit-(PatheticallyBad editor)     |
// |              developed by               |
// |                DDEZortep                |
// |                 email :                 |
// |ortepboulos@protonmail.com               |
// | Improved with Vi-like keybindings       |
// -------------------------------------------

#include <ncurses.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stack>

class TextEditor {
public:
    // Editor modes
    enum class EditorMode {
        NORMAL,
        INSERT,
        COMMAND,
        VISUAL,
        VISUAL_LINE,
        VISUAL_BLOCK
    };

    TextEditor(const std::string &fileName) 
        : cursorX(0), cursorY(0), offsetY(0), fileName(fileName), 
          mode(EditorMode::NORMAL), commandBuffer(""), 
          visualStartX(0), visualStartY(0), 
          repeatCount(0), lastCommand('\0') {
        initscr(); 
        raw();     
        keypad(stdscr, TRUE); 
        noecho();  
        start_color(); 
        initColors(); 
        if (!loadFile()) {
            lines.push_back(""); 
        }
    }

    ~TextEditor() {
        endwin(); 
    }

    void run() {
        while (true) {
            display();
            int ch = getch();

            // Mode-dependent input handling
            switch(mode) {
                case EditorMode::NORMAL:
                    handleNormalModeInput(ch);
                    break;
                case EditorMode::INSERT:
                    handleInsertModeInput(ch);
                    break;
                case EditorMode::COMMAND:
                    handleCommandModeInput(ch);
                    break;
                case EditorMode::VISUAL:
                case EditorMode::VISUAL_LINE:
                case EditorMode::VISUAL_BLOCK:
                    handleVisualModeInput(ch);
                    break;
            }
        }
    }

private:
    int cursorX, cursorY, offsetY;
    std::string fileName;
    std::vector<std::string> lines;
    EditorMode mode;
    std::string commandBuffer;
    
    // Visual mode selection tracking
    int visualStartX, visualStartY;
    
    // Repeat count and last command for . repeat functionality
    int repeatCount;
    char lastCommand;

    // Undo/Redo functionality
    std::stack<std::vector<std::string>> undoStack;
    std::stack<std::vector<std::string>> redoStack;

    // Color pair IDs
    const int LINE_NUMBER_COLOR = 1;
    const int STATUS_BAR_COLOR = 2;
    const int COMMAND_COLOR = 3;
    const int VISUAL_COLOR = 4;

    void initColors() {
        init_pair(LINE_NUMBER_COLOR, COLOR_BLACK, COLOR_WHITE);
        init_pair(STATUS_BAR_COLOR, COLOR_BLACK, COLOR_GREEN);
        init_pair(COMMAND_COLOR, COLOR_BLACK, COLOR_BLUE);
        init_pair(VISUAL_COLOR, COLOR_WHITE, COLOR_CYAN);
    }

    void handleNormalModeInput(int ch);
    void handleInsertModeInput(int ch);
    void handleCommandModeInput(int ch);
    void handleVisualModeInput(int ch);

    // New methods for enhanced Vi functionality
    void yankText();
    void deleteText();
    void pasteText();
    void changeText();
    void indentLine();
    void unindentLine();
    void searchText();
    void jumpToMatchingBracket();

    bool loadFile() {
        std::ifstream file(fileName);
        if (!file.is_open()) return false;

        lines.clear();
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return true;
    }

    void saveCurrentStateForUndo() {
        undoStack.push(lines);
        // Clear redo stack when a new action is performed
        while (!redoStack.empty()) redoStack.pop();
    }

    void undo() {
        if (!undoStack.empty()) {
            // Save current state to redo stack
            redoStack.push(lines);
            
            // Restore previous state
            lines = undoStack.top();
            undoStack.pop();

            // Adjust cursor if needed
            cursorY = std::min(cursorY, (int)lines.size() - 1);
            cursorX = std::min(cursorX, (int)lines[cursorY].length());
        }
    }

    void redo() {
        if (!redoStack.empty()) {
            // Save current state to undo stack
            undoStack.push(lines);
            
            // Restore next state
            lines = redoStack.top();
            redoStack.pop();

            // Adjust cursor if needed
            cursorY = std::min(cursorY, (int)lines.size() - 1);
            cursorX = std::min(cursorX, (int)lines[cursorY].length());
        }
    }

    bool saveFile() {
        std::ofstream file(fileName);
        if (!file.is_open()) {
            statusMessage("Error saving file!");
            return false;
        }

        for (const auto &line : lines) {
            file << line << '\n';
        }
        file.close();
        statusMessage("File saved successfully.");
        return true;
    }

    void insertChar(int ch) {
        lines[cursorY].insert(cursorX, 1, ch);
        cursorX++;
    }

    void backspace() {
        if (cursorX > 0) {
            lines[cursorY].erase(cursorX - 1, 1);
            cursorX--;
        } else if (cursorY > 0) {
            cursorX = lines[cursorY - 1].length();
            lines[cursorY - 1] += lines[cursorY];
            lines.erase(lines.begin() + cursorY);
            cursorY--;
            if (cursorY < offsetY) offsetY--;
        }
    }

    void newLine() {
        std::string newLine = lines[cursorY].substr(cursorX);
        lines[cursorY] = lines[cursorY].substr(0, cursorX);
        lines.insert(lines.begin() + cursorY + 1, newLine);
        cursorY++;
        cursorX = 0;
        if (cursorY >= offsetY + LINES - 2) offsetY++;
    }

    void moveUp() {
        if (cursorY > 0) {
            cursorY--;
            cursorX = std::min(cursorX, (int)lines[cursorY].length());
            if (cursorY < offsetY) offsetY--;
        }
    }

    void moveDown() {
        if (cursorY < (int)lines.size() - 1) {
            cursorY++;
            cursorX = std::min(cursorX, (int)lines[cursorY].length());
            if (cursorY >= offsetY + LINES - 2) offsetY++;
        }
    }

    void moveLeft() {
        if (cursorX > 0) {
            cursorX--;
        } else if (cursorY > 0) {
            cursorY--;
            cursorX = lines[cursorY].length();
            if (cursorY < offsetY) offsetY--;
        }
    }

    void moveRight() {
        if (cursorX < (int)lines[cursorY].length()) {
            cursorX++;
        } else if (cursorY < (int)lines.size() - 1) {
            cursorY++;
            cursorX = 0;
            if (cursorY >= offsetY + LINES - 2) offsetY++;
        }
    }

    void moveToNextWord() {
        while (cursorY < (int)lines.size() && 
               cursorX < (int)lines[cursorY].length() && 
               !std::isspace(lines[cursorY][cursorX])) {
            moveRight();
        }
        while (cursorY < (int)lines.size() && 
               cursorX < (int)lines[cursorY].length() && 
               std::isspace(lines[cursorY][cursorX])) {
            moveRight();
        }
    }

    void moveToPreviousWord() {
        while (cursorY > 0 || cursorX > 0) {
            moveLeft();
            if (cursorX > 0 && !std::isspace(lines[cursorY][cursorX-1])) {
                while (cursorX > 0 && !std::isspace(lines[cursorY][cursorX-1])) {
                    moveLeft();
                }
                break;
            }
        }
    }

    void moveToLineStart() {
        cursorX = 0;
    }

    void moveToLineEnd() {
        cursorX = lines[cursorY].length();
    }

    void moveToDocumentStart() {
        cursorY = 0;
        cursorX = 0;
        offsetY = 0;
    }

    void moveToDocumentEnd() {
        cursorY = lines.size() - 1;
        cursorX = lines[cursorY].length();
        offsetY = std::max(0, (int)lines.size() - (LINES - 2));
    }

    void display() {
        clear();

        int rows = LINES - 2;

        // Draw the visible lines of text with line numbers
        for (int i = 0; i < rows; ++i) {
            int lineIndex = offsetY + i;
            if (lineIndex < (int)lines.size()) {
                // Display line number
                attron(COLOR_PAIR(LINE_NUMBER_COLOR));
                std::ostringstream lineNumber;
                lineNumber << std::setw(4) << lineIndex + 1;
                mvprintw(i, 0, "%s ", lineNumber.str().c_str());
                attroff(COLOR_PAIR(LINE_NUMBER_COLOR));

                // Draw the actual line of text
                if (mode == EditorMode::VISUAL || 
                    mode == EditorMode::VISUAL_LINE || 
                    mode == EditorMode::VISUAL_BLOCK) {
                    bool isSelected = (lineIndex >= std::min(visualStartY, cursorY) && 
                                       lineIndex <= std::max(visualStartY, cursorY));
                    
                    if (isSelected) {
                        attron(COLOR_PAIR(VISUAL_COLOR));
                    }

                    for (int j = 0; j < (int)lines[lineIndex].length(); ++j) {
                        if (mode == EditorMode::VISUAL && 
                            j >= std::min(visualStartX, cursorX) && 
                            j <= std::max(visualStartX, cursorX)) {
                            if (isSelected) mvaddch(i, 5 + j, lines[lineIndex][j]);
                        } else if (mode == EditorMode::VISUAL_LINE && isSelected) {
                            mvaddch(i, 5 + j, lines[lineIndex][j]);
                        } else if (mode == EditorMode::VISUAL_BLOCK) {
                            if (j >= std::min(visualStartX, cursorX) && 
                                j <= std::max(visualStartX, cursorX)) {
                                mvaddch(i, 5 + j, lines[lineIndex][j]);
                            }
                        }
                    }

                    if (isSelected) {
                        attroff(COLOR_PAIR(VISUAL_COLOR));
                    }
                } else if (i == cursorY - offsetY) {
                    for (int j = 0; j < (int)lines[lineIndex].length(); ++j) {
                        if (j == cursorX) {
                            attron(A_REVERSE);
                            mvaddch(i, 5 + j, lines[lineIndex][j]);
                            attroff(A_REVERSE);
                        } else {
                            mvaddch(i, 5 + j, lines[lineIndex][j]);
                        }
                    }
                } else {
                    mvprintw(i, 5, "%s", lines[lineIndex].c_str());
                }
            }
        }

        // Draw the status bar
        attron(COLOR_PAIR(STATUS_BAR_COLOR));
        displayStatusBar();
        attroff(COLOR_PAIR(STATUS_BAR_COLOR));

        // If in command mode, display command buffer
        if (mode == EditorMode::COMMAND) {
            attron(COLOR_PAIR(COMMAND_COLOR));
            mvprintw(LINES - 1, 0, ":%s", commandBuffer.c_str());
            attroff(COLOR_PAIR(COMMAND_COLOR));
        }

        move(cursorY - offsetY, cursorX + 5);
        refresh();
    }

    void displayStatusBar() {
        std::ostringstream status;
        
        // Display current mode
        std::string modeStr;
        switch(mode) {
            case EditorMode::NORMAL: modeStr = "NORMAL"; break;
            case EditorMode::INSERT: modeStr = "INSERT"; break;
            case EditorMode::COMMAND: modeStr = "COMMAND"; break;
            case EditorMode::VISUAL: modeStr = "VISUAL"; break;
            case EditorMode::VISUAL_LINE: modeStr = "VISUAL LINE"; break;
            case EditorMode::VISUAL_BLOCK: modeStr = "VISUAL BLOCK"; break;
        }

        status << "Mode: " << modeStr << " | "
               << "Pos: (" << cursorY + 1 << "," << cursorX + 1 
               << ") | File: " << fileName;

        // Pad the status message to fit the terminal width
        std::string statusStr = status.str();
        if ((int)statusStr.length() < COLS) {
            statusStr += std::string(COLS - statusStr.length(), ' ');
        } else if ((int)statusStr.length() > COLS) {
            statusStr = statusStr.substr(0, COLS);
        }

        mvprintw(LINES - 2, 0, "%s", statusStr.c_str());
        clrtoeol();
    }

    void statusMessage(const std::string &message) {
        mvprintw(LINES - 1, 0, "%s", message.c_str());
        clrtoeol();
        refresh();
    }

    // Clipboard for yank/paste
    std::vector<std::string> clipboardLines;
    EditorMode clipboardType;

    int CTRL(char c) { return c & 0x1F; }
};

void TextEditor::searchText() {
    // Placeholder for search functionality
    //I don't got time for that lmao 
    statusMessage("Search not implemented yet.");
}

void TextEditor::jumpToMatchingBracket() {
    char currentChar = lines[cursorY][cursorX];
    char matchBracket = '\0';
    bool searchForward = true;

    switch(currentChar) {
        case '(': matchBracket = ')'; break;
        case '{': matchBracket = '}'; break;
        case '[': matchBracket = ']'; break;
        case ')': matchBracket = '('; searchForward = false; break;
        case '}': matchBracket = '{'; searchForward = false; break;
        case ']': matchBracket = '['; searchForward = false; break;
        default: return;
    }

    int depth = 1;
    int startY = cursorY, startX = cursorX;

    if (searchForward) {
        // Search forward
        for (int y = startY; y < (int)lines.size(); ++y) {
            int xStart = (y == startY) ? startX + 1 : 0;
            for (int x = xStart; x < (int)lines[y].length(); ++x) {
                if (lines[y][x] == currentChar) depth++;
                if (lines[y][x] == matchBracket) depth--;

                if (depth == 0) {
                    cursorY = y;
                    cursorX = x;
                    return;
                }
            }
        }
    } else {
        // Search backward
        for (int y = startY; y >= 0; --y) {
            int xEnd = (y == startY) ? startX - 1 : lines[y].length() - 1;
            for (int x = xEnd; x >= 0; --x) {
                if (lines[y][x] == currentChar) depth++;
                if (lines[y][x] == matchBracket) depth--;

                if (depth == 0) {
                    cursorY = y;
                    cursorX = x;
                    return;
                }
            }
        }
    }
}

void TextEditor::yankText() {
    saveCurrentStateForUndo();
    
    // Clear previous clipboard
    clipboardLines.clear();

    if (mode == EditorMode::VISUAL) {
        // Rectangular visual mode
        int startX = std::min(visualStartX, cursorX);
        int endX = std::max(visualStartX, cursorX);
        int startY = std::min(visualStartY, cursorY);
        int endY = std::max(visualStartY, cursorY);

        clipboardType = EditorMode::VISUAL;
        for (int y = startY; y <= endY; ++y) {
            std::string line = lines[y].substr(startX, endX - startX + 1);
            clipboardLines.push_back(line);
        }
    } else if (mode == EditorMode::VISUAL_LINE) {
        // Line visual mode
        int startY = std::min(visualStartY, cursorY);
        int endY = std::max(visualStartY, cursorY);

        clipboardType = EditorMode::VISUAL_LINE;
        for (int y = startY; y <= endY; ++y) {
            clipboardLines.push_back(lines[y]);
        }
    } else {
        // Normal mode, usually triggered by 'yy' command
        clipboardType = EditorMode::NORMAL;
        clipboardLines.push_back(lines[cursorY]);
    }

    mode = EditorMode::NORMAL;
    statusMessage("Text yanked.");
}

void TextEditor::deleteText() {
    saveCurrentStateForUndo();
    
    if (mode == EditorMode::VISUAL) {
        // Rectangular visual mode
        int startX = std::min(visualStartX, cursorX);
        int endX = std::max(visualStartX, cursorX);
        int startY = std::min(visualStartY, cursorY);
        int endY = std::max(visualStartY, cursorY);

        clipboardLines.clear();
        clipboardType = EditorMode::VISUAL;
        
        for (int y = endY; y >= startY; --y) {
            std::string line = lines[y];
            clipboardLines.insert(clipboardLines.begin(), 
                line.substr(startX, endX - startX + 1));
            
            lines[y].erase(startX, endX - startX + 1);
        }
    } else if (mode == EditorMode::VISUAL_LINE) {
        // Line visual mode
        int startY = std::min(visualStartY, cursorY);
        int endY = std::max(visualStartY, cursorY);

        clipboardType = EditorMode::VISUAL_LINE;
        clipboardLines.clear();
        
        for (int y = endY; y >= startY; --y) {
            clipboardLines.insert(clipboardLines.begin(), lines[y]);
            lines.erase(lines.begin() + y);
        }
        
        cursorY = std::min(startY, (int)lines.size() - 1);
        cursorX = 0;
    } else {
        // Normal mode, triggered by 'dd' or delete command
        clipboardLines.clear();
        clipboardType = EditorMode::NORMAL;
        
        if (lines.size() > 1) {
            clipboardLines.push_back(lines[cursorY]);
            lines.erase(lines.begin() + cursorY);
            
            if (cursorY >= (int)lines.size()) 
                cursorY = lines.size() - 1;
        }
    }

    mode = EditorMode::NORMAL;
}

void TextEditor::pasteText() {
    saveCurrentStateForUndo();

    if (clipboardLines.empty()) return;

    if (clipboardType == EditorMode::VISUAL_LINE) {
        // Paste lines after current line
        lines.insert(lines.begin() + cursorY + 1, 
                     clipboardLines.begin(), 
                     clipboardLines.end());
        cursorY += clipboardLines.size();
        cursorX = 0;
    } else if (clipboardType == EditorMode::VISUAL) {
        // Paste rectangular selection
        int pasteX = cursorX;
        for (int i = 0; i < (int)clipboardLines.size() && 
                        cursorY + i < (int)lines.size(); ++i) {
            if (pasteX + clipboardLines[i].length() > (int)lines[cursorY + i].length()) {
                lines[cursorY + i].resize(pasteX + clipboardLines[i].length());
            }
            
            lines[cursorY + i].replace(pasteX, 
                                       clipboardLines[i].length(), 
                                       clipboardLines[i]);
        }
    } else {
        // Paste after cursor position in the current line
        lines[cursorY].insert(cursorX, clipboardLines[0]);
        cursorX += clipboardLines[0].length();
    }
}

void TextEditor::changeText() {
    if (mode == EditorMode::VISUAL) {
        deleteText();
        mode = EditorMode::INSERT;
    } else {
        // In normal mode, typically 'cw' or 'cc'
        deleteText();
        mode = EditorMode::INSERT;
    }
}

void TextEditor::indentLine() {
    saveCurrentStateForUndo();
    lines[cursorY].insert(0, "    ");
    cursorX += 4;
}

void TextEditor::unindentLine() {
    saveCurrentStateForUndo();
    if (lines[cursorY].length() >= 4 && 
        lines[cursorY].substr(0, 4) == "    ") {
        lines[cursorY].erase(0, 4);
        cursorX = std::max(0, cursorX - 4);
    }
}

void TextEditor::handleVisualModeInput(int ch) {
    switch(ch) {
        case 27:  // ESC key
            mode = EditorMode::NORMAL;
            break;
        case 'y':
            yankText();
            break;
        case 'd':
            deleteText();
            break;
        case 'c':
            changeText();
            break;
        case '>':
            // Indent selected lines
            {
                int startY = std::min(visualStartY, cursorY);
                int endY = std::max(visualStartY, cursorY);
                for (int y = startY; y <= endY; ++y) {
                    saveCurrentStateForUndo();
                    lines[y].insert(0, "    ");
                }
                mode = EditorMode::NORMAL;
            }
            break;
        case '<':
            // Unindent selected lines
            {
                int startY = std::min(visualStartY, cursorY);
                int endY = std::max(visualStartY, cursorY);
                for (int y = startY; y <= endY; ++y) {
                    saveCurrentStateForUndo();
                    if (lines[y].length() >= 4 && 
                        lines[y].substr(0, 4) == "    ") {
                        lines[y].erase(0, 4);
                    }
                }
                mode = EditorMode::NORMAL;
            }
            break;
        
        // Movement keys for visual mode
        case 'h': moveLeft(); break;
        case 'l': moveRight(); break;
        case 'j': moveDown(); break;
        case 'k': moveUp(); break;
        case '0': moveToLineStart(); break;
        case '$': moveToLineEnd(); break;
        case 'w': moveToNextWord(); break;
        case 'b': moveToPreviousWord(); break;
    }
}

void TextEditor::handleNormalModeInput(int ch) {
    // Support for repeat count (chaining commands into eachother)
    if (std::isdigit(ch)) {
        repeatCount = repeatCount * 10 + (ch - '0');
        return;
    }

    // Repeat last command if no digit entered
    int iterations = std::max(1, repeatCount);
    repeatCount = 0;

    for (int i = 0; i < iterations; ++i) {
        switch(ch) {d
            case 'i': 
                mode = EditorMode::INSERT;
                break;
            case 'I':
                moveToLineStart();
                mode = EditorMode::INSERT;
                break;
            case 'a':
                if (cursorX < (int)lines[cursorY].length()) 
                    cursorX++;
                mode = EditorMode::INSERT;
                break;
            case 'A':
                moveToLineEnd();
                mode = EditorMode::INSERT;
                break;
            case 'k': moveUp(); break;
            case 'j': moveDown(); break;
            case 'h': moveLeft(); break;
            case 'l': moveRight(); break;
            case '0': moveToLineStart(); break;
            case '$': moveToLineEnd(); break;
            case 'w': moveToNextWord(); break;
            case 'b': moveToPreviousWord(); break;
            case 'g': {
                int nextCh = getch();
                if (nextCh == 'g') moveToDocumentStart();
            }
            break;
            case 'G': moveToDocumentEnd(); break;
            case 'x':
                // Delete character
                if (cursorX < (int)lines[cursorY].length()) {
                    saveCurrentStateForUndo();
                    lines[cursorY].erase(cursorX, 1);
                }
                break;
            case 'd': {
                int nextCh = getch();
                if (nextCh == 'd') deleteText();
            }
            break;
            case 'y': {
                int nextCh = getch();
                if (nextCh == 'y') yankText();
            }
            break;
            case 'c': {
                int nextCh = getch();
                if (nextCh == 'c') changeText();
            }
            break;
            case 'p': pasteText(); break;
            case 'u': undo(); break;
//            case CTRL('r'): redo(); break; 
// was having issues with this. note to self, fix later 
            case '%': jumpToMatchingBracket(); break;
            case '>>': indentLine(); break;
            case '<<': unindentLine(); break;
            case 'v': 
                mode = EditorMode::VISUAL;
                visualStartX = cursorX;
                visualStartY = cursorY;
                break;
            case 'V':
                mode = EditorMode::VISUAL_LINE;
                visualStartX = cursorX;
                visualStartY = cursorY;
                break;
            case ':':
                mode = EditorMode::COMMAND;
                commandBuffer.clear();
                break;
            case '/': searchText(); break;
            case 27: mode = EditorMode::NORMAL; break;
        }
    }
}

void TextEditor::handleInsertModeInput(int ch) {
    switch(ch) {
        case 27:  // ESC key
            mode = EditorMode::NORMAL;
            if (cursorX > 0) cursorX--;
            break;
        case KEY_BACKSPACE:
        case 127:
            backspace();
            break;
        case '\n':
            saveCurrentStateForUndo();
            newLine();
            break;
        case KEY_UP: moveUp(); break;
        case KEY_DOWN: moveDown(); break;
        case KEY_LEFT: moveLeft(); break;
        case KEY_RIGHT: moveRight(); break;
        default:
            saveCurrentStateForUndo();
            insertChar(ch);
            break;
    }
}

void TextEditor::handleCommandModeInput(int ch) {
    if (ch == '\n') {
        // Process command
        if (commandBuffer == "q") {
            endwin();
            exit(0);
        } else if (commandBuffer == "wq") {
            saveFile();
            endwin();
            exit(0);
        } else if (commandBuffer == "w") {
            saveFile();
        } else if (commandBuffer == "q!") {
            endwin();
            exit(0);
        } else if (commandBuffer == "u") {
            undo();
        } else if (commandBuffer == "redo") {
            redo();
        }
        mode = EditorMode::NORMAL;
        commandBuffer.clear();
    } else if (ch == 27) {  // ESC key
        mode = EditorMode::NORMAL;
        commandBuffer.clear();
    } else {
        commandBuffer += static_cast<char>(ch);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    TextEditor editor(argv[1]);
    editor.run();

    return 0;
}
