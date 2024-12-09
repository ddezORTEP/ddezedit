// -------------------------------------------
// |     PBedit-(PatheticallyBad editor)     |
// |              developped by              |
// |                DDEZortep                |
// |                 email :                 |
// |ortepboulos@protonmail.com               |
// | I don't recommend using this            |
// | go use nvim vim or emacs                |
// | stop reading this, code is under        |
// | feel free to get "inspired"             |  
// -------------------------------------------

#include <ncurses.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

class TextEditor {
public:
    TextEditor(const std::string &fileName) 
        : cursorX(0), cursorY(0), offsetY(0), fileName(fileName) {
        initscr(); // Initialize ncurses screen
        raw();     // Disable line buffering
        keypad(stdscr, TRUE); // Enable function and arrow keys
        noecho();  // Don't echo typed characters
        start_color(); // Initialize color functionality
        initColors(); // Initialize custom colors
        if (!loadFile()) {
            lines.push_back(""); // Start with an empty line if the file doesn't exist
        }
    }

    ~TextEditor() {
        endwin(); // End ncurses mode
    }

    void run() {
        while (true) {
            display();
            int ch = getch();
            if (ch == 27) { // ESC key to exit
                if (promptSave()) break;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                backspace();
            } else if (ch == '\n') {
                newLine();
            } else if (ch == KEY_UP) {
                moveUp();
            } else if (ch == KEY_DOWN) {
                moveDown();
            } else if (ch == KEY_LEFT) {
                moveLeft();
            } else if (ch == KEY_RIGHT) {
                moveRight();
            } else if (ch == CTRL('s')) { // CTRL+S to save
                saveFile();
            } else if (ch == CTRL('r')){ // CTRL +R to reload preview window
                system("/home/ortep/Docmuments/School/PP/3dracegame/reload.sh");
            } else {
                insertChar(ch);
            }
        }
    }

private:
    int cursorX, cursorY, offsetY;
    std::string fileName;
    std::vector<std::string> lines;

    // Color pair IDs
    const int LINE_NUMBER_COLOR = 1;
    const int STATUS_BAR_COLOR = 2;
 //for the r/unixporn fanboys to be happy they can customize the colors
    void initColors() {
        // Initialize color pairs
        init_pair(LINE_NUMBER_COLOR, COLOR_BLACK, COLOR_WHITE);  // Line numbers col1 bg on col2 fg
        init_pair(STATUS_BAR_COLOR, COLOR_BLACK, COLOR_GREEN);   // Status bar in col1 bg on col2 fg
    }

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

    bool promptSave() {
        mvprintw(LINES - 1, 0, "Save changes before exiting? (y/n): ");
        clrtoeol();
        refresh();
        int ch = getch();
        if (ch == 'y' || ch == 'Y') {
            saveFile();
        }
        return ch == 'n' || ch == 'N' || ch == 'y' || ch == 'Y';
    }

    void display() {
        clear();

        // Reserve the rows above the status bar
        int rows = LINES - 2;

        // Draw the visible lines of text with line numbers
        for (int i = 0; i < rows; ++i) {
            int lineIndex = offsetY + i;
            if (lineIndex < (int)lines.size()) {
                // Display line number with darker color
                attron(COLOR_PAIR(LINE_NUMBER_COLOR));
                std::ostringstream lineNumber;
                lineNumber << std::setw(4) << lineIndex + 1;
                mvprintw(i, 0, "%s ", lineNumber.str().c_str());
                attroff(COLOR_PAIR(LINE_NUMBER_COLOR));

                // Draw the actual line of text
                if (i == cursorY - offsetY) {
                    // Highlight the line with the cursor
                    for (int j = 0; j < (int)lines[lineIndex].length(); ++j) {
                        if (j == cursorX) {
                            attron(A_REVERSE); // Highlight the character at the cursor position
                            mvaddch(i, 5 + j, lines[lineIndex][j]); // 5 is the offset to account for line number
                            attroff(A_REVERSE); // Stop highlighting
                        } else {
                            mvaddch(i, 5 + j, lines[lineIndex][j]);
                        }
                    }
                } else {
                    mvprintw(i, 5, "%s", lines[lineIndex].c_str());
                }
            }
        }

        // Draw the status bar with different color
        attron(COLOR_PAIR(STATUS_BAR_COLOR));
        displayStatusBar();
        attroff(COLOR_PAIR(STATUS_BAR_COLOR));

        // Place the cursor at the correct position (it won't be visible due to highlighting idk why I hope this works)
        move(cursorY - offsetY, cursorX + 5); // Shift cursor by 5 to account for line number
        refresh();
    }

    void displayStatusBar() { //WORK WORK WORK WORK				
        // Bottom bar: Display controls and file information
        std::ostringstream status;
        status << "CTRL+S: Save | CTRL+R: Reload Preview | ESC: Exit | Cursor: (" 
               << cursorY + 1 << "," << cursorX + 1 
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

    int CTRL(char c) { return c & 0x1F; }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    TextEditor editor(argv[1]);
    editor.run();

    return 0;
}

