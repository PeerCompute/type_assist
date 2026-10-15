#include "textedit.h"
#include "filemanager.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QTextBlock>

TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent), m_popup(new SuggestionPopup(this))
{
    setAcceptRichText(false);
    viewport()->setMouseTracking(true);

    connect(m_popup, &SuggestionPopup::suggestionChosen, this, &TextEdit::onSuggestionChosen);
    connect(m_popup, &SuggestionPopup::addToDictionaryClicked, this, &TextEdit::onAddToDictionary);
    connect(this, &QTextEdit::cursorPositionChanged, this, [this]() {
        if (m_highlighter)
            m_highlighter->setCursorPosition(textCursor().blockNumber(), textCursor().positionInBlock());
    });
}

void TextEdit::setSpellChecker(ISpellChecker *checker) {
    m_spellChecker = checker;
    if (checker && !m_highlighter)
        m_highlighter = new TextHighlighter(document(), checker);
}

std::vector<std::string> TextEdit::getSuggestions() {
    if (!m_autoEngine) return {};
    m_autoEngine->resetToRoot();
    for (QChar c : extractPrefix()) m_autoEngine->processInput(c.toLatin1());
    return m_autoEngine->getSuggestions();
}

CheckResult TextEdit::checkWord(const QString &w, int k) {
    return m_spellChecker ? m_spellChecker->checkWord(w.toStdString(), k) : CheckResult{true, {}};
}

void TextEdit::insertWord(const QString &w) {
    if (w.isEmpty()) return;
    if (m_highlighter) m_highlighter->addWordToDictionary(w);
    else if (m_spellChecker) m_spellChecker->addToDictionary(w.toStdString());
    emit wordAddedToDictionary(w);
    emit statusMessage("'" + w + "' added to dictionary.");
    m_popup->hidePopup();
}

bool TextEdit::saveDocument(const QString &path) {
    bool ok = !path.isEmpty() && FileManager::saveFile(path, toPlainText());
    if (ok) document()->setModified(false);
    emit statusMessage(ok ? "Saved successfully." : "Error saving file.");
    return ok;
}

void TextEdit::keyPressEvent(QKeyEvent *e) {
    if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::Autocomplete && m_popup->handleKey(e))
        return;

    QTextEdit::keyPressEvent(e);

    const QString t = e->text();
    if (t.isEmpty() || !t[0].isLetter()) {
        m_popup->hidePopup();
        if (m_autoEngine) m_autoEngine->resetToRoot();
        if (m_highlighter) m_highlighter->rehighlight();
    } else {
        updateAutocomplete();
    }
}

void TextEdit::mousePressEvent(QMouseEvent *e) {
    QTextEdit::mousePressEvent(e);
    if (!showSpellCheckAt(e->pos()) && m_popup->isVisible())
        m_popup->hidePopup();
}

bool TextEdit::event(QEvent *e) {
    if (e->type() == QEvent::ToolTip) {
        auto *he = static_cast<QHelpEvent*>(e);
        if (showSpellCheckAt(he->pos())) return true;
    }
    return QTextEdit::event(e);
}

void TextEdit::onSuggestionChosen(const QString &w) {
    if (m_mode == SuggestionPopup::PopupMode::Autocomplete) {
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_prefix.length());
        QString rep = w;
        if (!m_prefix.isEmpty() && m_prefix[0].isUpper() && !rep.isEmpty()) rep[0] = rep[0].toUpper();
        c.insertText(rep);
        if (m_autoEngine) {
            m_autoEngine->updateFrequency(rep.toStdString());
            m_autoEngine->resetToRoot();
        }
    } else {
        m_misspelledCursor.insertText(w);
        if (m_highlighter) {
            m_highlighter->removeCachedEntry(m_misspelledWord);
            m_highlighter->rehighlight();
        }
    }
    m_popup->hidePopup();
}

void TextEdit::onAddToDictionary() {
    insertWord(m_misspelledWord);
}

QString TextEdit::extractPrefix() const {
    QTextCursor c = textCursor();
    QString text = c.block().text().left(c.positionInBlock());
    int i = text.length();
    while (i > 0 && text[i - 1].isLetter()) --i;
    return text.mid(i);
}

void TextEdit::updateAutocomplete() {
    m_prefix = extractPrefix();
    if (m_prefix.isEmpty() || !m_autoEngine) {
        m_popup->hidePopup();
        return;
    }
    m_autoEngine->resetToRoot();
    for (QChar c : m_prefix) m_autoEngine->processInput(c.toLatin1());

    QStringList list;
    for (const auto &s : m_autoEngine->getSuggestions()) list << QString::fromStdString(s);
    if (list.isEmpty()) { m_popup->hidePopup(); return; }

    m_mode = SuggestionPopup::PopupMode::Autocomplete;
    m_popup->showSuggestions(list, mapToGlobal(cursorRect().bottomLeft() + QPoint(0, 4)), m_mode);
}

bool TextEdit::showSpellCheckAt(const QPoint &pos) {
    QTextCursor c = cursorForPosition(pos);
    c.select(QTextCursor::WordUnderCursor);
    QString w = c.selectedText().trimmed();
    if (w.isEmpty() || !m_highlighter) return false;

    QStringList sugs = m_highlighter->getSuggestionsFor(w);
    if (sugs.isEmpty() && (!m_spellChecker || m_spellChecker->checkWord(w.toStdString(), 5).isCorrect))
        return false;

    m_mode = SuggestionPopup::PopupMode::SpellCheck;
    m_misspelledWord = w;
    m_misspelledCursor = c;
    m_popup->showSuggestions(sugs, mapToGlobal(cursorRect(c).bottomLeft() + QPoint(0, 4)), m_mode);
    return true;
}
