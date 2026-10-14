#include "suggestionpopup.h"
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>

SuggestionPopup::SuggestionPopup(QWidget* parent)
    : QWidget(parent, Qt::ToolTip)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_list = new QListWidget(this);
    m_list->setFocusPolicy(Qt::NoFocus);
    // the list never has focus, so make the selected row clearly visible
    m_list->setStyleSheet(
        "QListWidget::item:selected { background: #3d8bfd; color: white; }");

    m_addToDictButton = new QPushButton("+ Add to Dictionary", this);
    m_addToDictButton->setFocusPolicy(Qt::NoFocus);
    m_addToDictButton->hide();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(m_list);
    layout->addWidget(m_addToDictButton);

    connect(m_list, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
                emit suggestionChosen(item->text());
                hidePopup();
            });

    connect(m_addToDictButton, &QPushButton::clicked, this,
            [this]() {
                emit addToDictionaryClicked();
                hidePopup();
            });
}

void SuggestionPopup::showSuggestions(const QStringList& suggestions,
                                      const QPoint& globalPos,
                                      PopupMode mode)
{
    if (suggestions.isEmpty()) {
        hidePopup();
        return;
    }

    m_mode = mode;
    m_list->clear();
    m_list->addItems(suggestions);
    m_list->setCurrentRow(-1);   // nothing selected until the user presses Down

    m_addToDictButton->setVisible(mode == PopupMode::SpellCheck);

    int rows = qMin(static_cast<int>(suggestions.size()), 6);
    m_list->setFixedHeight(m_list->sizeHintForRow(0) * rows + 4);
    setFixedWidth(180);
    adjustSize();

    move(globalPos);
    show();
}

void SuggestionPopup::hidePopup()
{
    hide();
}

void SuggestionPopup::moveSelection(int step)
{
    int count = m_list->count();
    if (count == 0)
        return;

    int row = m_list->currentRow();
    if (row < 0)
        row = (step > 0) ? 0 : count - 1;      // first Down -> top, first Up -> bottom
    else
        row = (row + step + count) % count;    // wrap around

    m_list->setCurrentRow(row);
}

bool SuggestionPopup::handleKey(QKeyEvent* event)
{
    // keyboard selection is for autocomplete only; the hover popup is mouse-driven
    if (!isVisible() || m_mode != PopupMode::Autocomplete)
        return false;

    switch (event->key()) {
    case Qt::Key_Down:
        moveSelection(+1);
        return true;
    case Qt::Key_Up:
        moveSelection(-1);
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        if (m_list->currentRow() >= 0) {       // only if the user picked a row
            emit suggestionChosen(m_list->currentItem()->text());
            hidePopup();
            return true;
        }
        return false;                          // nothing selected: Enter works normally
    case Qt::Key_Escape:
        hidePopup();
        return true;
    default:
        return false;
    }
}