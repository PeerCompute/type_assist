#ifndef TEXTHIGHLIGHTER_H
#define TEXTHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QHash>
#include <QStringList>
#include "ispellchecker.h"

class TextHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TextHighlighter(QTextDocument* document, ISpellChecker* checker);

    QStringList getSuggestionsFor(const QString& word) const;
    void addWordToDictionary(const QString& word);
    void removeCachedEntry(const QString& word);
    void setCursorPosition(int blockNumber, int positionInBlock);

protected:
    void highlightBlock(const QString& text) override;

private:
    ISpellChecker* m_spellChecker;
    QHash<QString, QStringList> m_suggestionCache;
    QTextCharFormat m_misspelledFormat;
    int m_cursorBlockNumber = -1;
    int m_cursorPositionInBlock = -1;

    static const int k = 5;   // number of corrections requested per word
};

#endif