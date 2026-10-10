#include "texthighlighter.h"
#include <QTextDocument>
#include <QTextCursor>

TextHighlighter::TextHighlighter(QTextDocument* document, ISpellChecker* checker)
    : QSyntaxHighlighter(document), m_spellChecker(checker)
{
    m_misspelledFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_misspelledFormat.setUnderlineColor(Qt::red);
}

void TextHighlighter::highlightBlock(const QString& text) {
    int cursorPos = -1;
    if (currentBlock().blockNumber() == m_cursorBlockNumber) {
        cursorPos = m_cursorPositionInBlock;
    }

    int start = 0;
    int length = text.length();

    for (int i = 0; i <= length; ++i) {
        bool atEnd = (i == length);
        bool isLetter = !atEnd && text[i].isLetter();

        if (!isLetter) {
            if (i > start) {
                QString word = text.mid(start, i - start);
                bool cursorInsideWord = (cursorPos >= start && cursorPos <= i);

                if (!cursorInsideWord) {
                    CheckResult result = m_spellChecker->checkWord(word.toStdString(), k);
                    if (!result.isCorrect) {
                        setFormat(start, i - start, m_misspelledFormat);
                        QStringList suggestions;
                        for (const auto& s : result.corrections)
                            suggestions << QString::fromStdString(s);
                        m_suggestionCache[word] = suggestions;
                    }
                }
            }
            start = i + 1;
        }
    }
}

QStringList TextHighlighter::getSuggestionsFor(const QString& word) const {
    return m_suggestionCache.value(word);
}

void TextHighlighter::addWordToDictionary(const QString& word) {
    m_spellChecker->addToDictionary(word.toStdString());
    m_suggestionCache.remove(word);
    rehighlight();
}

void TextHighlighter::removeCachedEntry(const QString& word) {
    m_suggestionCache.remove(word);
}

void TextHighlighter::setCursorPosition(int blockNumber, int positionInBlock) {
    m_cursorBlockNumber = blockNumber;
    m_cursorPositionInBlock = positionInBlock;
}