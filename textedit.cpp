#include "textedit.h"
#include "filemanager.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QTextBlock>
#include <QTextLayout>
#include <QMenu>
#include <QFont>
#include <QCursor>
#include <QScrollBar>
#include <QFontMetrics>


namespace {
bool feedEngine(IAutocompleteEngine *engine, const QString &prefix)
{
    engine->resetToRoot();
    const QString low = prefix.toLower();
    if (low.isEmpty())
        return false;
    for (QChar c : low) {
        if (c.unicode() < 'a' || c.unicode() > 'z')
            return false;
    }
    for (QChar c : low) {
        engine->processInput(c.toLatin1());
    }
    return true;
}
}

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

    // Disable visual word-wrap so horizontal scrollbar governs width,
    // then set document width to exactly MAX_CHARS_PER_LINE char-widths.
    setLineWrapMode(QTextEdit::NoWrap);
    {
        QFontMetrics fm(f);
        // Use space width as reference; +2*margin for document margins
        int maxPxWidth = fm.horizontalAdvance(' ') * MAX_CHARS_PER_LINE
                         + static_cast<int>(document()->documentMargin()) * 2;
        document()->setPageSize(QSizeF(maxPxWidth, -1));
    }

    // Dark scrollbar styling
    setStyleSheet(
        "QScrollBar:vertical {"
        "  background: #1a1a1a;"
        "  width: 10px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #4a4a4a;"
        "  border-radius: 5px;"
        "  min-height: 24px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #6a6a6a;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "  background: #1a1a1a;"
        "  height: 10px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #4a4a4a;"
        "  border-radius: 5px;"
        "  min-width: 24px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #6a6a6a;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
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
    connect(this, &QTextEdit::textChanged, this, &TextEdit::updateCursorInfo);
}

void TextEdit::setSpellChecker(ISpellChecker *checker)
{
    m_spellChecker = checker;
    if (checker && !m_highlighter)
        m_highlighter = new TextHighlighter(document(), checker);
}

std::vector<std::string> TextEdit::getSuggestions()
{
    if (!m_autoEngine || !feedEngine(m_autoEngine, extractPrefix()))
        return {};
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

void TextEdit::zoomInStep()
{
    zoomIn(1);
    m_zoomDelta++;
    emit zoomChanged(100 + m_zoomDelta * 10);
}

void TextEdit::zoomOutStep()
{
    if (m_zoomDelta > -7) {
        zoomOut(1);
        m_zoomDelta--;
        emit zoomChanged(100 + m_zoomDelta * 10);
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

    if (m_popup->isVisible() && m_popup->handleKey(e))
        return;

    // ── Hard 1000-char-per-line limit ────────────────────────────────────────
    // Check BEFORE letting QTextEdit insert the character so we can block it.
    const int key = e->key();
    const bool isPrintable = !e->text().isEmpty() && e->text()[0].isPrint();
    const bool isTab       = (key == Qt::Key_Tab);

    if (isPrintable || isTab) {
        QTextCursor c = textCursor();
        QString lineText = c.block().text();

        // Calculate effective length of the current line with tab expansion
        int effectiveLen = 0;
        for (QChar ch : lineText) {
            if (ch == '\t')
                effectiveLen += TAB_SIZE - (effectiveLen % TAB_SIZE);
            else
                ++effectiveLen;
        }

        // How many columns will the incoming key consume?
        int incoming = isTab ? (TAB_SIZE - (effectiveLen % TAB_SIZE)) : 1;

        if (effectiveLen + incoming > MAX_CHARS_PER_LINE) {
            // Line is full – silently swallow the key
            e->accept();
            emit statusMessage(QString("Line limit: max %1 chars per line reached.").arg(MAX_CHARS_PER_LINE));
            return;
        }
    }
    // ─────────────────────────────────────────────────────────────────────────

    QTextEdit::keyPressEvent(e);

    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta || key == Qt::Key_CapsLock)
        return;

    if (key == Qt::Key_Backspace || key == Qt::Key_Delete || (!e->text().isEmpty() && e->text()[0].isLetter())) {
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
    if (!showSpellCheckAt(e->pos()) && m_popup->isVisible()) {
        m_closeTimer->stop();
        m_popup->hidePopup();
    }
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    QTextEdit::mouseMoveEvent(e);
    if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::Autocomplete)
        return;

    QTextCursor hc;
    QString w = wordAt(e->pos(), &hc);
    int hBlock = hc.block().blockNumber();
    int hStart = hc.selectionStart() - hc.block().position();
    if (!w.isEmpty() && m_highlighter && !m_highlighter->isIgnoredOnce(hBlock, hStart) && m_highlighter->isMisspelled(w)) {
        m_closeTimer->stop();
        if (w != m_hoveredWord || !m_popup->isVisible()) {
            m_hoveredWord = w;
            m_hoverPos = e->pos();
            m_hoverTimer->start(200);
        }
    } else {
        m_hoverTimer->stop();
        m_hoveredWord.clear();
        if (m_popup->isVisible() && m_mode == SuggestionPopup::PopupMode::SpellCheck && !m_popup->isMouseOver() && !m_closeTimer->isActive()) {
            m_closeTimer->start(400);
        }
    }
}

void TextEdit::onHoverTimeout()
{
    if (!m_hoveredWord.isEmpty())
        showSpellCheckAt(m_hoverPos);
}

void TextEdit::onCloseTimeout()
{
    if (m_popup->isMouseOver())
        return;

    QPoint localPos = viewport()->mapFromGlobal(QCursor::pos());
    if (viewport()->rect().contains(localPos) && wordAt(localPos) == m_misspelledWord)
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

void TextEdit::onSuggestionChosen(const QString &w)
{
    if (m_mode == SuggestionPopup::PopupMode::Autocomplete) {
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_prefix.length());
        QString rep = w;
        if (!m_prefix.isEmpty() && m_prefix[0].isUpper() && !rep.isEmpty())
            rep[0] = rep[0].toUpper();
        c.insertText(rep);
        if (m_autoEngine) {
            m_autoEngine->updateFrequency(rep.toLower().toStdString());
            m_autoEngine->resetToRoot();
        }
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

QString TextEdit::extractPrefix() const
{
    QTextCursor c = textCursor();
    QString text = c.block().text().left(c.positionInBlock());
    int i = text.length();
    while (i > 0 && text[i - 1].isLetter())
        --i;
    return text.mid(i);
}

void TextEdit::updateAutocomplete()
{
    m_prefix = extractPrefix();
    if (m_prefix.isEmpty() || !m_autoEngine || !feedEngine(m_autoEngine, m_prefix)) {
        m_popup->hidePopup();
        return;
    }

    QStringList list;
    for (const auto &s : m_autoEngine->getSuggestions())
        list << QString::fromStdString(s);

    if (list.isEmpty()) {
        m_popup->hidePopup();
        return;
    }

    m_mode = SuggestionPopup::PopupMode::Autocomplete;
    QPoint globalPos = viewport()->mapToGlobal(cursorRect().bottomLeft() + QPoint(0, 4));
    m_popup->showSuggestions(list, globalPos, m_mode);
}

bool TextEdit::showSpellCheckAt(const QPoint &pos)
{
    QTextCursor c;
    QString w = wordAt(pos, &c);
    if (w.isEmpty() || !m_highlighter)
        return false;

    int block = c.block().blockNumber();
    int start = c.selectionStart() - c.block().position();
    if (m_highlighter->isIgnoredOnce(block, start))
        return false;

    if (!m_highlighter->isMisspelled(w))
        return false;

    m_mode = SuggestionPopup::PopupMode::SpellCheck;
    m_misspelledWord = w;
    m_misspelledCursor = c;
    QPoint globalPos = viewport()->mapToGlobal(cursorRect(c).bottomLeft() + QPoint(0, 4));
    m_popup->showSuggestions(m_highlighter->getSuggestionsFor(w), globalPos, m_mode);
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