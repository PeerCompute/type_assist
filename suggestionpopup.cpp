#include "suggestionpopup.h"
#include <QListWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QGraphicsDropShadowEffect>

SuggestionPopup::SuggestionPopup(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);

    m_container = new QWidget(this);
    m_container->setObjectName("popupContainer");
    m_container->setStyleSheet(
        "#popupContainer {"
        "  background-color: #ffffff;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "}"
        "QListWidget {"
        "  border: none;"
        "  background: transparent;"
        "  font-family: 'Segoe UI', -apple-system, sans-serif;"
        "  font-size: 13px;"
        "  outline: none;"
        "  padding: 0px;"
        "}"
        "QListWidget::item {"
        "  height: 28px;"
        "  padding: 0px 8px;"
        "  margin: 1px 0px;"
        "  border-radius: 4px;"
        "  color: #1e293b;"
        "}"
        "QListWidget::item:hover:!selected {"
        "  background-color: #f1f5f9;"
        "  color: #0f172a;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #0078d4;"
        "  color: #ffffff;"
        "  font-weight: 500;"
        "}"
        "QListWidget::item:selected:hover {"
        "  background-color: #0078d4;"
        "  color: #ffffff;"
        "}"
    );

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(14);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 3);
    m_container->setGraphicsEffect(shadow);

    m_list = new QListWidget(m_container);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* innerLayout = new QVBoxLayout(m_container);
    innerLayout->setContentsMargins(4, 4, 4, 4);
    innerLayout->setSpacing(0);
    innerLayout->addWidget(m_list);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 8, 8, 8); // Shadow margin
    outerLayout->addWidget(m_container);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            emit suggestionChosen(item->text());
            hidePopup();
        }
    });
}

void SuggestionPopup::enterEvent(QEnterEvent* event)
{
    QWidget::enterEvent(event);
    emit mouseEntered();
}

void SuggestionPopup::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    emit mouseLeft();
}

bool SuggestionPopup::isMouseOver() const
{
    return isVisible() && geometry().contains(QCursor::pos());
}

void SuggestionPopup::showSuggestions(const QStringList& suggestions,
                                      const QPoint& globalPos,
                                      PopupMode mode)
{
    QStringList displayList = suggestions;
    if (displayList.size() > 3)
        displayList = displayList.mid(0, 3);

    if (displayList.isEmpty()) {
        hidePopup();
        return;
    }

    m_mode = mode;
    m_list->clear();
    m_list->addItems(displayList);

    // Default to the first suggestion so Down arrow immediately highlights the next item
    m_list->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);

    const int count = displayList.size();
    const int itemHeight = 30;
    const int listHeight = itemHeight * count;
    m_list->setFixedHeight(listHeight);

    const int containerHeight = listHeight + 8; // 4px top + 4px bottom padding
    m_container->setFixedHeight(containerHeight);

    const int totalHeight = containerHeight + 16; // 8px top + 8px bottom shadow margin
    const int totalWidth = 150 + 16;
    setFixedSize(totalWidth, totalHeight);

    // Screen bounds clamping
    QPoint pos = globalPos;
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (screen) {
        QRect screenGeo = screen->availableGeometry();
        if (pos.x() + width() > screenGeo.right())
            pos.setX(screenGeo.right() - width());
        if (pos.x() < screenGeo.left())
            pos.setX(screenGeo.left());

        if (pos.y() + height() > screenGeo.bottom())
            pos.setY(pos.y() - height() - 24);
    }

    move(pos);
    show();
    raise();
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
    if (row < 0) {
        row = (step > 0) ? 0 : count - 1;
    } else {
        row = (row + step + count) % count;
    }

    m_list->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
}

bool SuggestionPopup::handleKey(QKeyEvent* event)
{
    if (!isVisible())
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
        if (m_list->currentRow() >= 0) {
            emit suggestionChosen(m_list->currentItem()->text());
        } else if (m_list->count() > 0) {
            emit suggestionChosen(m_list->item(0)->text());
        }
        hidePopup();
        return true;
    case Qt::Key_Escape:
        hidePopup();
        return true;
    default:
        return false;
    }
}