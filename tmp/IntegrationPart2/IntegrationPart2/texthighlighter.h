#ifndef TEXTHIGHLIGHTER_H
#define TEXTHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QHash>
#include <QSet>
#include <QStringList>
#include "ispellchecker.h"

class TextHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TextHighlighter(QTextDocument* document, ISpellChecker* checker);

    QStringList getSuggestionsFor(const QString& word) const;
    bool isMisspelled(const QString& word) const;
    void addWordToDictionary(const QString& word);  // persists to dictionary
    void ignoreWord(const QString& word);            // session-only, no persist
    void setCursorPosition(int blockNumber, int positionInBlock);

protected:
    void highlightBlock(const QString& text) override;

private:
    void refreshBlock(int blockNumber);

    ISpellChecker* m_spellChecker = nullptr;

    mutable QHash<QString, QStringList> m_suggestionCache; // misspelled word -> corrections
    mutable QSet<QString> m_validWords;                    // words already confirmed correct

    QTextCharFormat m_misspelledFormat;

    int m_cursorBlockNumber = -1;
    int m_cursorPositionInBlock = -1;
};

#endif // TEXTHIGHLIGHTER_H