#ifndef ISPELLCHECKER_H
#define ISPELLCHECKER_H

#include <string>
#include <vector>

struct CheckResult {
    bool isCorrect;
    std::vector<std::string> corrections;
};

class ISpellChecker {
public:
    virtual ~ISpellChecker() = default;
    virtual CheckResult checkWord(const std::string& word, int k) = 0;
    virtual void addToDictionary(const std::string& word) = 0;
};

#endif