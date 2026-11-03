#ifndef TEXTEDIT_H
#define TEXTEDIT_H

// Tab stop width in columns used for visual column counting
static constexpr int TAB_SIZE = 4;

#include <QTextEdit>
#include <QStringList>
#include <QTimer>
#include <vector>
#include <string>

#include "suggestionpopup.h"
#include "texthighlighter.h"
#include "iAutocompleteEngine.h"
#include "ispellchecker.h"

class TextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = nullptr);

    void setAutocompleteEngine(IAutocompleteEngine *engine) { m_autoEngine = engine; }
    void setSpellChecker(ISpellChecker *checker);

    // Required Interfaces
    std::vector<std::string> getSuggestions();
    CheckResult checkWord(const QString &word, int k = 5);
    void insertWord(const QString &word);
    bool saveDocument(const QString &filePath);

    // Zoom Controls (30% lower bound, 500% upper bound)
    static constexpr int MIN_ZOOM_PERCENT = 30;
    static constexpr int MAX_ZOOM_PERCENT = 500;
    static constexpr int MIN_ZOOM_DELTA   = (MIN_ZOOM_PERCENT - 100) / 10; // -7
    static constexpr int MAX_ZOOM_DELTA   = (MAX_ZOOM_PERCENT - 100) / 10; // 40
    void zoomInStep();
    void zoomOutStep();
    void resetZoomStep();
    int zoomPercent() const { return 100 + m_zoomDelta * 10; }

signals:
    void statusMessage(const QString &msg);
    void wordAddedToDictionary(const QString &word);
    void zoomChanged(int percent);
    void cursorInfoChanged(int line, int col, int wordCount, int charCount);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void showEvent(QShowEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

private slots:
    void onSuggestionChosen(const QString &word);
    void onHoverTimeout();
    void onCloseTimeout();
    void updateCursorInfo();

private:
    QString extractPrefix() const;
    QString wordAt(const QPoint &pos, QTextCursor *cursorOut = nullptr) const;
    bool findMisspelledAt(const QPoint &pos, QString &wordOut, QTextCursor &cursorOut) const;
    void updateAutocomplete();
    bool showSpellCheckAt(const QPoint &pos);
    // Returns the visual (wrapped) line number and visual column (tab-expanded)
    // for the current text cursor position.
    std::pair<int,int> getVisualLineAndCol() const;

    IAutocompleteEngine *m_autoEngine = nullptr;
    ISpellChecker *m_spellChecker = nullptr;
    TextHighlighter *m_highlighter = nullptr;
    SuggestionPopup *m_popup = nullptr;

    QTimer *m_hoverTimer = nullptr;
    QTimer *m_closeTimer = nullptr;
    QPoint m_hoverPos;
    QString m_hoveredWord;

    QString m_prefix;
    QString m_misspelledWord;
    QTextCursor m_misspelledCursor;
    SuggestionPopup::PopupMode m_mode = SuggestionPopup::PopupMode::Autocomplete;

    int m_zoomDelta = 0;
};

#endif // TEXTEDIT_H
