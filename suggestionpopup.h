#ifndef SUGGESTIONPOPUP_H
#define SUGGESTIONPOPUP_H

#include <QWidget>
#include <QStringList>
#include <QPoint>

class QListWidget;
class QKeyEvent;
class QEnterEvent;

class SuggestionPopup : public QWidget {
    Q_OBJECT

public:
    enum class PopupMode { Autocomplete, SpellCheck };

    explicit SuggestionPopup(QWidget* parent = nullptr);

    void showSuggestions(const QStringList& suggestions,
                         const QPoint& globalPos,
                         PopupMode mode);
    void hidePopup();

    bool handleKey(QKeyEvent* event);
    PopupMode mode() const { return m_mode; }
    bool isMouseOver() const;

signals:
    void suggestionChosen(const QString& word);
    void addToDictionaryRequested();
    void ignoreRequested();
    void mouseEntered();
    void mouseLeft();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void moveSelection(int step);

    QWidget* m_container = nullptr;
    QListWidget* m_list = nullptr;
    PopupMode m_mode = PopupMode::Autocomplete;
};

#endif // SUGGESTIONPOPUP_H