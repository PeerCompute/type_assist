#ifndef SUGGESTIONPOPUP_H
#define SUGGESTIONPOPUP_H

#include <QWidget>
#include <QStringList>
#include <QPoint>

class QListWidget;
class QPushButton;
class QKeyEvent;

class SuggestionPopup : public QWidget {
    Q_OBJECT

public:
    enum class PopupMode { Autocomplete, SpellCheck };

    explicit SuggestionPopup(QWidget* parent = nullptr);

    void showSuggestions(const QStringList& suggestions,
                         const QPoint& globalPos,
                         PopupMode mode);
    void hidePopup();

    // TextEdit forwards key presses here; returns true if the popup used the key
    bool handleKey(QKeyEvent* event);

signals:
    void suggestionChosen(const QString& word);
    void addToDictionaryClicked();

private:
    void moveSelection(int step);

    QListWidget* m_list;
    QPushButton* m_addToDictButton;
    PopupMode m_mode = PopupMode::Autocomplete;
};

#endif