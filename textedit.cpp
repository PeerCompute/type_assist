#include "textedit.h"
#include "filemanager.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QShowEvent>
#include <QTextBlock>
#include <QTextLayout>
#include <QMenu>
#include <QFont>
#include <QCursor>
#include <QScrollBar>
#include <QFontMetrics>


TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent),
      m_popup(new SuggestionPopup(this)),
      m_hoverTimer(new QTimer(this)),
      m_closeTimer(new QTimer(this))
{
    setAcceptRichText(false);
    viewport()->setMouseTracking(true);

    QFont f("Segoe UI", 11);
    setFont(f);
    document()->setDefaultFont(f);
    document()->setDocumentMargin(8);

    // Responsive word-wrap: wrap at window width like Microsoft Notepad, no horizontal scrollbar
    setLineWrapMode(QTextEdit::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Thin modern scrollbar styling (matching Windows Notepad)
    setStyleSheet(
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 5px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  border-radius: 2px;"
        "  min-height: 24px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(255, 255, 255, 0.45);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: transparent;"
        "}"
        "QScrollBar:horizontal {"
        "  background: transparent;"
        "  height: 5px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  border-radius: 2px;"
        "  min-width: 24px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: rgba(255, 255, 255, 0.45);"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "  background: transparent;"
        "}"
    );

    m_hoverTimer->setSingleShot(true);
    m_closeTimer->setSingleShot(true);
    connect(m_hoverTimer, &QTimer::timeout, this, &TextEdit::onHoverTimeout);
    connect(m_closeTimer, &QTimer::timeout, this, &TextEdit::onCloseTimeout);

    connect(m_popup, &SuggestionPopup::suggestionChosen, this, &TextEdit::onSuggestionChosen);
    connect(m_popup, &SuggestionPopup::mouseEntered, m_closeTimer, &QTimer::stop);
    connect(m_popup, &SuggestionPopup::mouseLeft, this, [this]() {
        if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::SpellCheck)
            m_closeTimer->start(350);
    });

    // SpellCheck popup actions
    connect(m_popup, &SuggestionPopup::addToDictionaryRequested, this, [this]() {
        if (!m_misspelledWord.isEmpty())
            insertWord(m_misspelledWord);
    });
    connect(m_popup, &SuggestionPopup::ignoreOnceRequested, this, [this]() {
        if (m_highlighter && !m_misspelledWord.isEmpty()) {
            int block = m_misspelledCursor.block().blockNumber();
            int start = m_misspelledCursor.selectionStart() - m_misspelledCursor.block().position();
            m_highlighter->ignoreWordOnce(block, start);
        }
        m_closeTimer->stop();
        m_popup->hidePopup();
    });
    connect(m_popup, &SuggestionPopup::ignoreAllRequested, this, [this]() {
        if (m_highlighter && !m_misspelledWord.isEmpty())
            m_highlighter->ignoreWordAll(m_misspelledWord);
        m_closeTimer->stop();
        m_popup->hidePopup();
    });
    connect(m_popup, &SuggestionPopup::ignoreRequested, this, [this]() {
        if (m_highlighter && !m_misspelledWord.isEmpty())
            m_highlighter->ignoreWordAll(m_misspelledWord);
        m_closeTimer->stop();
        m_popup->hidePopup();
    });

    connect(this, &QTextEdit::cursorPositionChanged, this, [this]() {
        if (m_highlighter)
            m_highlighter->setCursorPosition(textCursor().blockNumber(), textCursor().positionInBlock());
        updateCursorInfo();
    });
    connect(this, &QTextEdit::textChanged, this, [this]() {
        if (document() && document()->documentLayout())
            document()->documentLayout()->documentSize();
        updateCursorInfo();
    });

    auto dismissPopupOnScroll = [this]() {
        if (m_popup && m_popup->isVisible()) {
            m_hoverTimer->stop();
            m_closeTimer->stop();
            m_popup->hidePopup();
        }
    };
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, dismissPopupOnScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, dismissPopupOnScroll);
}

void TextEdit::setSpellChecker(ISpellChecker *checker)
{
    m_spellChecker = checker;
    if (checker && !m_highlighter)
        m_highlighter = new TextHighlighter(document(), checker);
}

std::vector<std::string> TextEdit::getSuggestions()
{
    QString context, prefix;
    if (!m_autoEngine || !suggestionContext(context, prefix))
        return {};
    m_autoEngine->setContext(context.toStdString());
    return m_autoEngine->getSuggestions();
}

CheckResult TextEdit::checkWord(const QString &w, int k)
{
    return m_spellChecker ? m_spellChecker->checkWord(w.toStdString(), k) : CheckResult{true, {}};
}

void TextEdit::insertWord(const QString &w)
{
    if (w.isEmpty())
        return;
    if (m_highlighter)
        m_highlighter->addWordToDictionary(w);
    else if (m_spellChecker)
        m_spellChecker->addToDictionary(w.toStdString());

    if (m_autoEngine)
        m_autoEngine->addWord(w.toLower().toStdString());

    emit wordAddedToDictionary(w);
    emit statusMessage("'" + w + "' added to dictionary.");
    m_closeTimer->stop();
    m_popup->hidePopup();
}

bool TextEdit::saveDocument(const QString &path)
{
    bool ok = !path.isEmpty() && FileManager::saveFile(path, toPlainText());
    if (ok)
        document()->setModified(false);
    emit statusMessage(ok ? "Saved successfully." : "Error saving file.");
    return ok;
}

void TextEdit::hidePopup()
{
    if (m_popup)
        m_popup->hidePopup();
}

void TextEdit::zoomInStep()
{
    // Hard upper limit: 500%
    if (zoomPercent() < MAX_ZOOM_PERCENT) {
        zoomIn(1);
        m_zoomDelta++;
        emit zoomChanged(zoomPercent());
    }
}

void TextEdit::zoomOutStep()
{
    // Hard lower limit: 30%
    if (zoomPercent() > MIN_ZOOM_PERCENT) {
        zoomOut(1);
        m_zoomDelta--;
        emit zoomChanged(zoomPercent());
    }
}

void TextEdit::resetZoomStep()
{
    if (m_zoomDelta > 0) {
        zoomOut(m_zoomDelta);
    } else if (m_zoomDelta < 0) {
        zoomIn(-m_zoomDelta);
    }
    m_zoomDelta = 0;
    emit zoomChanged(100);
}

void TextEdit::wheelEvent(QWheelEvent *e)
{
    if (m_popup && m_popup->isVisible()) {
        m_hoverTimer->stop();
        m_closeTimer->stop();
        m_popup->hidePopup();
    }

    if (e->modifiers() & Qt::ControlModifier) {
        if (e->angleDelta().y() > 0)
            zoomInStep();
        else if (e->angleDelta().y() < 0)
            zoomOutStep();
        e->accept();
        return;
    }
    QTextEdit::wheelEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        const int k = e->key();
        if (k == Qt::Key_Plus || k == Qt::Key_Equal || e->text() == "+" || e->text() == "=") {
            zoomInStep(); e->accept(); return;
        } else if (k == Qt::Key_Minus || k == Qt::Key_Underscore || e->text() == "-" || e->text() == "_") {
            zoomOutStep(); e->accept(); return;
        } else if (k == Qt::Key_0) {
            resetZoomStep(); e->accept(); return;
        }
    }

    const int key = e->key();

    if (m_popup->isVisible()) {
        // Next-word suggestions appear after every space, so Enter must keep
        // inserting a newline there (Tab still accepts the suggestion).
        if (m_mode == SuggestionPopup::PopupMode::Autocomplete && m_nextWordMode &&
            (key == Qt::Key_Return || key == Qt::Key_Enter))
            m_popup->hidePopup();
        else if (m_popup->handleKey(e))
            return;
    }

    // A word is finished by space / punctuation / Enter: learn it before the
    // key is inserted (the prefix is still in front of the cursor).
    if (!(e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {
        const QString t = e->text();
        const bool boundary =
            key == Qt::Key_Return || key == Qt::Key_Enter ||
            (!t.isEmpty() && (t[0].isSpace() || t[0].isPunct()));
        if (boundary)
            learnTypedWord();
    }

    QTextEdit::keyPressEvent(e);

    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta || key == Qt::Key_CapsLock)
        return;

    // Letters / backspace: completions of the typed prefix.
    // Space (or backspacing to a space): next word from the bigram / trigram.
    if (key == Qt::Key_Backspace || key == Qt::Key_Delete || key == Qt::Key_Space ||
        (!e->text().isEmpty() && e->text()[0].isLetter())) {
        updateAutocomplete();
    } else {
        m_popup->hidePopup();
        if (m_autoEngine)
            m_autoEngine->resetToRoot();
    }
}


void TextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        if (!showSpellCheckAt(e->pos()) && m_popup->isVisible()) {
            m_closeTimer->stop();
            m_popup->hidePopup();
        }
        return;
    }

    QTextEdit::mousePressEvent(e);
    if (!showSpellCheckAt(e->pos()) && m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::SpellCheck) {
        m_closeTimer->stop();
        m_popup->hidePopup();
    }
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    QTextEdit::mouseMoveEvent(e);
    if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::Autocomplete)
        return;

    QString w;
    QTextCursor hc;
    if (findMisspelledAt(e->pos(), w, hc)) {
        int hBlock = hc.block().blockNumber();
        int hStart = hc.selectionStart() - hc.block().position();
        if (m_highlighter && !m_highlighter->isIgnoredOnce(hBlock, hStart)) {
            m_closeTimer->stop();
            if (w != m_hoveredWord || !m_popup->isVisible()) {
                m_hoveredWord = w;
                m_hoverPos = e->pos();
                m_hoverTimer->start(200);
            }
            return;
        }
    }

    m_hoverTimer->stop();
    m_hoveredWord.clear();
    if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::SpellCheck && !m_popup->isMouseOver() && !m_closeTimer->isActive()) {
        m_closeTimer->start(400);
    }
}

void TextEdit::onHoverTimeout()
{
    if (window() && window()->isMinimized()) {
        if (m_popup)
            m_popup->hidePopup();
        return;
    }
    if (!m_hoveredWord.isEmpty())
        showSpellCheckAt(m_hoverPos);
}

void TextEdit::onCloseTimeout()
{
    if (m_popup->isMouseOver())
        return;

    QPoint localPos = viewport()->mapFromGlobal(QCursor::pos());
    QString w;
    QTextCursor c;
    if (viewport()->rect().contains(localPos) && findMisspelledAt(localPos, w, c) && w == m_misspelledWord)
        return;

    m_popup->hidePopup();
}

void TextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    // Spell suggestions are shown in the hover popup — right-click shows standard menu only
    QMenu *menu = createStandardContextMenu();
    menu->exec(e->globalPos());
    delete menu;
}

void TextEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    if (m_popup)
        m_popup->hidePopup();
}

void TextEdit::focusOutEvent(QFocusEvent *e)
{
    QTextEdit::focusOutEvent(e);
    if (m_popup && !m_popup->isMouseOver())
        m_popup->hidePopup();
}

void TextEdit::showEvent(QShowEvent *e)
{
    QTextEdit::showEvent(e);
    if (window())
        window()->installEventFilter(this);
}

bool TextEdit::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == window()) {
        if (e->type() == QEvent::Move || e->type() == QEvent::Resize ||
            e->type() == QEvent::WindowStateChange || e->type() == QEvent::WindowDeactivate ||
            e->type() == QEvent::Hide) {
            if (m_popup)
                m_popup->hidePopup();
        }
    }
    return QTextEdit::eventFilter(obj, e);
}

void TextEdit::onSuggestionChosen(const QString &w)
{
    if (m_mode == SuggestionPopup::PopupMode::Autocomplete) {
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_prefix.length());
        QString rep = w;
        if (!m_prefix.isEmpty() && m_prefix[0].isUpper() && !rep.isEmpty())
            rep[0] = rep[0].toUpper();

        // Add a space so the next-word suggestions can follow straight away.
        const QTextBlock blk = c.block();
        const int after = c.selectionEnd() - blk.position();
        const bool spaceFollows = after < blk.text().size() && blk.text()[after].isSpace();
        QString inserted = rep;
        if (!spaceFollows)
            inserted += QLatin1Char(' ');
        c.insertText(inserted);

        if (m_autoEngine) {
            m_autoEngine->updateFrequency(rep.toLower().toStdString());
            m_autoEngine->resetToRoot();
        }
        // The popup is hidden right after this slot returns, so ask for the
        // next-word suggestions once the event loop is back.
        QTimer::singleShot(0, this, &TextEdit::updateAutocomplete);
    } else {
        m_misspelledCursor.insertText(w);
    }
    m_closeTimer->stop();
    m_popup->hidePopup();
}

QString TextEdit::wordAt(const QPoint &pos, QTextCursor *cursorOut) const
{
    QTextCursor c = cursorForPosition(pos);
    c.select(QTextCursor::WordUnderCursor);
    if (cursorOut)
        *cursorOut = c;
    return c.selectedText().trimmed();
}

bool TextEdit::findMisspelledAt(const QPoint &pos, QString &wordOut, QTextCursor &cursorOut) const
{
    if (!m_highlighter)
        return false;

    QTextCursor c = cursorForPosition(pos);
    QTextBlock block = c.block();
    if (!block.isValid() || !block.layout())
        return false;

    int posInBlock = c.position() - block.position();

    for (const auto &range : block.layout()->formats()) {
        if (range.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
            if (posInBlock >= range.start && posInBlock <= range.start + range.length) {
                wordOut = block.text().mid(range.start, range.length);
                cursorOut = QTextCursor(block);
                cursorOut.setPosition(block.position() + range.start);
                cursorOut.setPosition(block.position() + range.start + range.length, QTextCursor::KeepAnchor);
                return true;
            }
        }
    }
    return false;
}

QString TextEdit::extractPrefix() const
{
    QTextCursor c = textCursor();
    QString text = c.block().text().left(c.positionInBlock());
    int i = text.length();
    while (i > 0 && text[i - 1].isLetter())
        --i;
    return text.mid(i);
}

// Works out what the n-gram engine should be asked.
// context: the current sentence up to the cursor (ends with the typed letters,
//          or with a space when predicting the next word).
// prefix:  the letters typed so far (empty in next-word mode).
// Returns false when no suggestions should be shown.
bool TextEdit::suggestionContext(QString &context, QString &prefix) const
{
    context.clear();
    prefix.clear();

    QTextCursor c = textCursor();
    if (c.hasSelection())
        return false;

    const QString text = c.block().text();
    const int pos = c.positionInBlock();

    // Cursor in the middle of a word: no suggestions.
    if (pos < text.size() && text[pos].isLetter())
        return false;

    QString before = text.left(pos);
    if (before.isEmpty())
        return false;

    // Only the end of the line matters for a trigram model.
    if (before.size() > 200)
        before = before.right(200);

    // A sentence end resets the context (the corpus is sentence based).
    const int cut = qMax(before.lastIndexOf(QLatin1Char('.')),
                    qMax(before.lastIndexOf(QLatin1Char('!')),
                         before.lastIndexOf(QLatin1Char('?'))));
    if (cut >= 0)
        before = before.mid(cut + 1);

    if (before.isEmpty())
        return false;

    auto isAscii = [](QChar ch) {
        const ushort u = ch.unicode();
        return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
    };

    if (before[before.size() - 1].isSpace()) {
        // Next-word mode: needs at least one previous word as context.
        bool hasLetter = false;
        for (const QChar ch : before) {
            if (isAscii(ch)) {
                hasLetter = true;
                break;
            }
        }
        if (!hasLetter)
            return false;
    } else {
        // Prefix mode: the word being typed must be letters only.
        int start = before.size();
        while (start > 0 && !before[start - 1].isSpace())
            --start;

        const QString chunk = before.mid(start);
        for (const QChar ch : chunk) {
            if (!isAscii(ch))
                return false;
        }
        prefix = chunk;
    }

    context = before;
    return true;
}

// Teach the engine the word that is about to be finished (called on a
// space / punctuation / Enter key, before the key is inserted).
void TextEdit::learnTypedWord()
{
    QString context, prefix;
    if (!m_autoEngine || !suggestionContext(context, prefix) || prefix.isEmpty())
        return;

    const QString word = prefix.toLower();
    const int end = textCursor().position();

    // Same word finished at the same place again (space, backspace, space).
    if (end == m_lastLearnedEnd && word == m_lastLearnedWord)
        return;

    m_lastLearnedEnd = end;
    m_lastLearnedWord = word;

    m_autoEngine->setContext(context.toStdString());
    m_autoEngine->updateFrequency(word.toStdString());
}

void TextEdit::updateAutocomplete()
{
    QString context, prefix;
    if (!m_autoEngine || !suggestionContext(context, prefix)) {
        m_prefix.clear();
        m_popup->hidePopup();
        return;
    }

    m_prefix = prefix;

    if (window() && window()->isMinimized()) {
        m_popup->hidePopup();
        return;
    }

    QRect cr = cursorRect();
    if (!viewport()->rect().intersects(cr)) {
        m_popup->hidePopup();
        return;
    }

    m_autoEngine->setContext(context.toStdString());

    const QString typed = prefix.toLower();
    QStringList list;
    for (const auto &s : m_autoEngine->getSuggestions()) {
        const QString w = QString::fromStdString(s);
        if (!typed.isEmpty() && w == typed)
            continue; // don't suggest what is already typed
        list << w;
    }

    if (list.isEmpty()) {
        m_popup->hidePopup();
        return;
    }

    m_mode = SuggestionPopup::PopupMode::Autocomplete;
    m_nextWordMode = prefix.isEmpty();
    QPoint globalPos = viewport()->mapToGlobal(cr.bottomLeft() + QPoint(0, 4));
    m_popup->showSuggestions(list, globalPos, m_mode, QString(), cr.height());
}

bool TextEdit::showSpellCheckAt(const QPoint &pos)
{
    if (window() && window()->isMinimized()) {
        m_popup->hidePopup();
        return false;
    }

    QString w;
    QTextCursor c;
    if (!findMisspelledAt(pos, w, c) || !m_highlighter)
        return false;

    QRect cr = cursorRect(c);
    if (!viewport()->rect().intersects(cr)) {
        m_popup->hidePopup();
        return false;
    }

    int block = c.block().blockNumber();
    int start = c.selectionStart() - c.block().position();
    if (m_highlighter->isIgnoredOnce(block, start))
        return false;

    m_mode = SuggestionPopup::PopupMode::SpellCheck;
    m_misspelledWord = w;
    m_misspelledCursor = c;
    QPoint globalPos = viewport()->mapToGlobal(cr.bottomLeft() + QPoint(0, 4));
    m_popup->showSuggestions(m_highlighter->getSuggestionsFor(w), globalPos, m_mode, w, cr.height());
    return true;
}

// ── Visual line/column via QTextLayout ───────────────────────────────────────
// Walks every block before the current one, summing their visual (wrapped) line
// counts, then adds the visual sub-line of the cursor inside its own block.
// Column is tab-expanded: a \t advances to the next TAB_SIZE boundary.
std::pair<int,int> TextEdit::getVisualLineAndCol() const
{
    QTextCursor c = textCursor();
    QTextBlock  curBlock = c.block();
    int posInBlock = c.positionInBlock();

    // Sum visual lines for all blocks before the current one
    int visualLine = 1;
    QTextBlock b = document()->begin();
    while (b.isValid() && b != curBlock) {
        // Ensure layout is up to date before querying
        if (b.layout())
            visualLine += b.layout()->lineCount();
        else
            ++visualLine; // fallback: treat as single line
        b = b.next();
    }

    // Find which visual sub-line inside the current block the cursor sits on
    int subLine = 0;
    if (curBlock.isValid() && curBlock.layout() && curBlock.layout()->lineCount() > 0) {
        QTextLine tl = curBlock.layout()->lineForTextPosition(posInBlock);
        if (tl.isValid())
            subLine = tl.lineNumber(); // 0-indexed
    }
    visualLine += subLine; // result is 1-based total visual line

    // Compute tab-expanded column for the portion of the block before the cursor
    QString lineText = curBlock.text().left(posInBlock);
    int visualCol = 1;
    // If cursor is on a sub-line (soft-wrapped), skip chars before that sub-line
    if (subLine > 0 && curBlock.layout() && curBlock.layout()->lineCount() > subLine) {
        QTextLine prevLine = curBlock.layout()->lineAt(subLine - 1);
        if (prevLine.isValid()) {
            // Characters up to textStart of current sub-line don't contribute
            int subLineStart = curBlock.layout()->lineAt(subLine).textStart();
            lineText = curBlock.text().mid(subLineStart, posInBlock - subLineStart);
        }
    }
    for (QChar ch : lineText) {
        if (ch == '\t')
            visualCol += TAB_SIZE - ((visualCol - 1) % TAB_SIZE);
        else
            ++visualCol;
    }

    return {visualLine, visualCol};
}

void TextEdit::updateCursorInfo()
{
    auto [visualLine, visualCol] = getVisualLineAndCol();

    QString text = toPlainText();
    int words = 0;
    bool inWord = false;
    for (QChar ch : text) {
        if (ch.isLetterOrNumber()) {
            if (!inWord) { inWord = true; ++words; }
        } else {
            inWord = false;
        }
    }
    emit cursorInfoChanged(visualLine, visualCol, words, text.length());
}
