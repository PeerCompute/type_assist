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
        if (!item) return;
        const QString role = item->data(Qt::UserRole).toString();
        if (role == QStringLiteral("separator")) return;
        if (role == QStringLiteral("add")) {
            emit addToDictionaryRequested();
        } else if (role == QStringLiteral("ignore_once")) {
            emit ignoreOnceRequested();
        } else if (role == QStringLiteral("ignore_all") || role == QStringLiteral("ignore")) {
            emit ignoreAllRequested();
        } else {
            emit suggestionChosen(item->text().trimmed());
        }
        hidePopup();
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

    // Add suggestion items, tagged with role
    for (const QString& s : displayList) {
        auto* item = new QListWidgetItem(s);
        item->setData(Qt::UserRole, QStringLiteral("suggestion"));
        m_list->addItem(item);
    }

    // SpellCheck mode: add separator, Ignore Once, Ignore All, and Add to Dictionary rows
    if (mode == PopupMode::SpellCheck) {
        auto* sep = new QListWidgetItem("\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500");
        sep->setFlags(Qt::NoItemFlags);
        sep->setForeground(QColor("#cbd5e1"));
        sep->setSizeHint(QSize(0, 14));
        sep->setData(Qt::UserRole, QStringLiteral("separator"));
        m_list->addItem(sep);

        auto* ignOnceItem = new QListWidgetItem("  Ignore Once");
        ignOnceItem->setForeground(QColor("#64748b"));
        ignOnceItem->setData(Qt::UserRole, QStringLiteral("ignore_once"));
        m_list->addItem(ignOnceItem);

        auto* ignAllItem = new QListWidgetItem("  Ignore All");
        ignAllItem->setForeground(QColor("#64748b"));
        ignAllItem->setData(Qt::UserRole, QStringLiteral("ignore_all"));
        m_list->addItem(ignAllItem);

        auto* addItem = new QListWidgetItem("  + Add to Dictionary");
        addItem->setForeground(QColor("#0078d4"));
        addItem->setData(Qt::UserRole, QStringLiteral("add"));
        m_list->addItem(addItem);
    }

    // Pre-select first suggestion
    m_list->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);

    const int sugCount = displayList.size();
    const int itemH    = 30;
    int listHeight     = itemH * sugCount;
    if (mode == PopupMode::SpellCheck)
        listHeight += 14 + itemH + itemH + itemH; // separator(14px) + Ignore Once(30px) + Ignore All(30px) + Add(30px)
    m_list->setFixedHeight(listHeight);

    const int containerHeight = listHeight + 8;
    m_container->setFixedHeight(containerHeight);

    const int totalHeight = containerHeight + 16;
    const int totalWidth  = (mode == PopupMode::SpellCheck ? 190 : 150) + 16;
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

    // Skip non-selectable separator items
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_list->item(row);
        if (!item || item->data(Qt::UserRole).toString() == QStringLiteral("separator"))
            row = (row + step + count) % count;
        else
            break;
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
    case Qt::Key_Tab: {
        QListWidgetItem* cur = (m_list->currentRow() >= 0)
                                   ? m_list->currentItem()
                                   : (m_list->count() > 0 ? m_list->item(0) : nullptr);
        if (cur) {
            const QString role = cur->data(Qt::UserRole).toString();
            if (role == QStringLiteral("add"))
                emit addToDictionaryRequested();
            else if (role == QStringLiteral("ignore_once"))
                emit ignoreOnceRequested();
            else if (role == QStringLiteral("ignore_all") || role == QStringLiteral("ignore"))
                emit ignoreAllRequested();
            else if (role != QStringLiteral("separator"))
                emit suggestionChosen(cur->text().trimmed());
        }
        hidePopup();
        return true;
    }
    case Qt::Key_Escape:
        hidePopup();
        return true;
    default:
        return false;
    }
}