#include "Autocomplete.h"

#include <cstdlib>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
int readKey() { return _getch(); }
void clearScreen() { std::system("cls"); }
#else
#include <termios.h>
#include <unistd.h>
int readKey() {
    termios oldSettings{};
    if (tcgetattr(STDIN_FILENO, &oldSettings) != 0) {
        return getchar();
    }

    termios newSettings = oldSettings;
    newSettings.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    newSettings.c_cc[VMIN] = 1;
    newSettings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newSettings) != 0) {
        return getchar();
    }

    const int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
    return ch;
}
void clearScreen() { std::system("clear"); }
#endif

int main() {
    Autocomplete autocomplete;
    if (!autocomplete.loadDictionary("dictionary.txt")) {
        std::cerr << "Failed to load dictionary.txt\n";
        return 1;
    }

    // A missing frequency file is expected on the first run.
    autocomplete.loadFrequencies("frequencies.txt");
    autocomplete.rebuildTopK();

    std::stack<TrieNode*> nodeStack;
    nodeStack.push(autocomplete.getTrie().getRoot());
    std::string input;

    std::cout << "Autocomplete started!\n"
              << "Type letters to see suggestions.\n"
              << "Backspace: delete | Enter: record typed word | Tab: accept first suggestion | Esc: save and exit\n";

    while (true) {
        clearScreen();
        std::cout << "Prefix: " << input << "\n\nSuggestions:\n";

        TrieNode* current = nodeStack.top();
        const std::vector<std::string> suggestions =
            (!input.empty() && current != nullptr)
                ? autocomplete.getTopKSuggestionsAtNode(current)
                : std::vector<std::string>{};

        for (const std::string& word : suggestions) {
            std::cout << "  " << word
                      << " - Frequency: " << autocomplete.getFrequency(word) << '\n';
        }
        if (suggestions.empty()) {
            std::cout << "  (no suggestions)\n";
        }
        std::cout << "\nEnter: record typed word | Tab: record first suggestion | Esc: exit\n";

        const int ch = readKey();
        if (ch == 27) {
            if (!autocomplete.saveFrequencies("frequencies.txt")) {
                std::cerr << "Warning: could not save frequencies.txt\n";
            }
            break;
        }
        if (ch == 8 || ch == 127) {
            if (!input.empty()) {
                input.pop_back();
                nodeStack.pop();
            }
            continue;
        }
        if (ch == 13 || ch == 10) {
            if (!input.empty()) {
                autocomplete.recordWord(input);
            }
            input.clear();
            while (nodeStack.size() > 1) {
                nodeStack.pop();
            }
            continue;
        }
        if (ch == 9) {
            if (!suggestions.empty()) {
                autocomplete.recordWord(suggestions.front());
                input.clear();
                while (nodeStack.size() > 1) {
                    nodeStack.pop();
                }
            }
            continue;
        }
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            const char typedChar = static_cast<char>(
                (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch);
            TrieNode* nextNode = autocomplete.getTrie().getChild(nodeStack.top(), typedChar);
            input += typedChar;
            nodeStack.push(nextNode);
        }
    }

    return 0;
}
