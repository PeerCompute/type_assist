#include "texthighlighter.h"
#include <QTextDocument>
#include <QTextBlock>

TextHighlighter::TextHighlighter(QTextDocument* document, ISpellChecker* checker)
    : QSyntaxHighlighter(document), m_spellChecker(checker)
{
    m_misspelledFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_misspelledFormat.setUnderlineColor(QColor(239, 68, 68)); // Clean vivid red
}

void TextHighlighter::highlightBlock(const QString& text)
{
    if (!m_spellChecker)
        return;

    int cursorPos = -1;
    if (currentBlock().blockNumber() == m_cursorBlockNumber)
        cursorPos = m_cursorPositionInBlock;

    int start = 0;
    const int length = text.length();

    for (int i = 0; i <= length; ++i) {
        bool isWordChar = false;
        if (i < length) {
            QChar ch = text[i];
            if (ch.isLetter()) {
                isWordChar = true;
            } else if (ch == '\'' && i > start && i + 1 < length && text[i + 1].isLetter()) {
                isWordChar = true;
            }
        }

        if (isWordChar)
            continue;

        const int wordLen = i - start;
        if (wordLen >= 2) {
            // If the cursor is currently inside or at the end of this word, the user is still typing it.
            // Skip spellchecking so incomplete words are not underlined.
            if (cursorPos >= start && cursorPos <= i) {
                start = i + 1;
                continue;
            }

            const QString word = text.mid(start, wordLen);
            if (m_suggestionCache.contains(word)) {
                setFormat(start, wordLen, m_misspelledFormat);
            } else if (m_validWords.contains(word)) {
                // Correct word, do nothing
            } else {
                const CheckResult result = m_spellChecker->checkWord(word.toStdString(), 5);
                if (result.isCorrect) {
                    m_validWords.insert(word);
                } else {
                    QStringList suggestions;
                    for (const auto& s : result.corrections)
                        suggestions << QString::fromStdString(s);
                    m_suggestionCache.insert(word, suggestions);
                    setFormat(start, wordLen, m_misspelledFormat);
                }
            }
        }
        start = i + 1;
    }
}

bool TextHighlighter::isMisspelled(const QString& word) const
{
    if (word.length() < 2)
        return false;
    if (m_suggestionCache.contains(word) || m_suggestionCache.contains(word.toLower()))
        return true;
    if (m_validWords.contains(word) || m_validWords.contains(word.toLower()))
        return false;
    if (m_spellChecker) {
        const CheckResult result = m_spellChecker->checkWord(word.toStdString(), 5);
        if (result.isCorrect) {
            m_validWords.insert(word);
            return false;
        } else {
            QStringList suggestions;
            for (const auto& s : result.corrections)
                suggestions << QString::fromStdString(s);
            m_suggestionCache.insert(word, suggestions);
            return true;
        }
    }
    return false;
}

QStringList TextHighlighter::getSuggestionsFor(const QString& word) const
{
    if (m_suggestionCache.contains(word))
        return m_suggestionCache.value(word);
    if (m_suggestionCache.contains(word.toLower()))
        return m_suggestionCache.value(word.toLower());
    if (m_spellChecker) {
        const CheckResult result = m_spellChecker->checkWord(word.toStdString(), 5);
        if (!result.isCorrect) {
            QStringList suggestions;
            for (const auto& s : result.corrections)
                suggestions << QString::fromStdString(s);
            m_suggestionCache.insert(word, suggestions);
            return suggestions;
        }
    }
    return {};
}

void TextHighlighter::addWordToDictionary(const QString& word)
{
    if (m_spellChecker)
        m_spellChecker->addToDictionary(word.toStdString());

    m_suggestionCache.remove(word);
    m_suggestionCache.remove(word.toLower());
    m_validWords.insert(word);
    m_validWords.insert(word.toLower());

    rehighlight();
}

void TextHighlighter::ignoreWord(const QString& word)
{
    // Remove from bad-word cache so the underline disappears
    m_suggestionCache.remove(word);
    m_suggestionCache.remove(word.toLower());
    // Mark as valid for this session only (not persisted to spell checker)
    m_validWords.insert(word);
    m_validWords.insert(word.toLower());
    rehighlight();
}

void TextHighlighter::setCursorPosition(int blockNumber, int positionInBlock)
{
    int prevBlock = m_cursorBlockNumber;
    m_cursorBlockNumber = blockNumber;
    m_cursorPositionInBlock = positionInBlock;

    if (prevBlock >= 0 && prevBlock != blockNumber) {
        refreshBlock(prevBlock);
    }
    refreshBlock(blockNumber);
}

void TextHighlighter::refreshBlock(int blockNumber)
{
    if (blockNumber < 0 || !document())
        return;
    const QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (block.isValid())
        rehighlightBlock(block);
}