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
            // If the cursor is currently inside or at the end of this word, and no non-alphabetical
            // character has been typed after it yet (i == length), the user is still actively typing it.
            // Skip spellchecking so words are only underlined once a non-alphabetical character is typed.
            if (cursorPos >= start && cursorPos <= i && i == length) {
                start = i + 1;
                continue;
            }

            const QString word = text.mid(start, wordLen);
            const QString lower = word.toLower();
            const int blockNum = currentBlock().blockNumber();
            const bool isIgnoredOnce = m_ignoredOncePositions.contains({blockNum, start});

            if (isIgnoredOnce || m_validWords.contains(word) || m_validWords.contains(lower)) {
                // Correct or ignored word, do nothing
            } else if (m_suggestionCache.contains(word) || m_suggestionCache.contains(lower)) {
                setFormat(start, wordLen, m_misspelledFormat);
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

void TextHighlighter::ignoreWordOnce(int blockNumber, int positionInBlock)
{
    m_ignoredOncePositions.insert({blockNumber, positionInBlock});
    refreshBlock(blockNumber);
}

void TextHighlighter::ignoreWordAll(const QString& word)
{
    // Remove from bad-word cache so the underline disappears
    m_suggestionCache.remove(word);
    m_suggestionCache.remove(word.toLower());
    // Mark as valid for this session only (not persisted to spell checker)
    m_validWords.insert(word);
    m_validWords.insert(word.toLower());
    rehighlight();
}

void TextHighlighter::ignoreWord(const QString& word)
{
    ignoreWordAll(word);
}

bool TextHighlighter::isIgnoredOnce(int blockNumber, int positionInBlock) const
{
    return m_ignoredOncePositions.contains({blockNumber, positionInBlock});
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