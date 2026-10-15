#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>
#include <QStringList>
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

    // Worklet 1 Required Interfaces
    std::vector<std::string> getSuggestions();
    CheckResult checkWord(const QString &word, int k = 5);
    void insertWord(const QString &word);
    bool saveDocument(const QString &filePath);

signals:
    void statusMessage(const QString &msg);
    void wordAddedToDictionary(const QString &word);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    bool event(QEvent *e) override;

private slots:
    void onSuggestionChosen(const QString &word);
    void onAddToDictionary();

private:
    QString extractPrefix() const;
    void updateAutocomplete();
    bool showSpellCheckAt(const QPoint &pos);

    IAutocompleteEngine *m_autoEngine = nullptr;
    ISpellChecker *m_spellChecker = nullptr;
    TextHighlighter *m_highlighter = nullptr;
    SuggestionPopup *m_popup = nullptr;

    QString m_prefix;
    QString m_misspelledWord;
    QTextCursor m_misspelledCursor;
    SuggestionPopup::PopupMode m_mode = SuggestionPopup::PopupMode::Autocomplete;
};

#endif // TEXTEDIT_H
