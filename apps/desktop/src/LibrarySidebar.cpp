#include "LibrarySidebar.hpp"

#include "Icons.hpp"
#include "Theme.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

#include <optional>

namespace Pocket::App {

class CategoryRow final : public QWidget {
    Q_OBJECT

public:
    CategoryRow(const QString& label, const QString& category, QWidget* parent, std::optional<Icons::Name> icon = std::nullopt)
        : QWidget(parent)
        , m_category(category)
    {
        setFixedHeight(30);
        setFocusPolicy(Qt::StrongFocus);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 8, 0);
        layout->setSpacing(8);
        m_marker = new QWidget(this);
        m_marker->setFixedWidth(2);
        m_iconName = icon;
        if (icon) {
            m_icon = new QLabel(this);
            m_icon->setPixmap(Icons::pixmap(*icon, Theme::textSecondary(), 15));
            layout->addWidget(m_icon);
        }
        m_label = new QLabel(label, this);
        m_count = new QLabel("0", this);
        m_count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(m_marker);
        layout->addWidget(m_label, 1);
        layout->addWidget(m_count);
        refreshStyle();
    }

    QString category() const
    {
        return m_category;
    }

    void setCount(int count)
    {
        m_count->setText(QString::number(count));
    }

    void setActive(bool active)
    {
        m_active = active;
        refreshStyle();
    }

signals:
    void activated(const QString& category);
    void moveRequested(int offset);

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            setFocus(Qt::MouseFocusReason);
            emit activated(m_category);
        }
        QWidget::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
            emit activated(m_category);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Up) {
            emit moveRequested(-1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            emit moveRequested(1);
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    void refreshStyle()
    {
        // Active state is the accent bar plus brighter text. No filled rectangle:
        // the sidebar should not compete with the covers.
        QColor marker = Theme::accent();
        marker.setAlphaF(0.85);
        const QColor primary = Theme::textPrimary();
        const QColor secondary = Theme::textSecondary();
        setStyleSheet(QString("CategoryRow { background: transparent; border: 1px solid transparent; border-radius: 5px; }"
                              "CategoryRow:hover { background: %1; }"
                              "CategoryRow:focus { border-color: %2; }"
                              "QLabel { background: transparent; color: %3; }"
                              "QWidget#marker { background: %4; border-radius: 1px; }")
                          .arg(Theme::rgba(Theme::surfaceHover()),
                               Theme::accent().name(),
                               m_active ? primary.name() : secondary.name(),
                               m_active ? marker.name(QColor::HexArgb) : QStringLiteral("transparent")));
        m_marker->setObjectName("marker");
        if (m_icon && m_iconName) {
            m_icon->setPixmap(Icons::pixmap(*m_iconName, m_active ? primary : secondary, 15));
        }
    }

    QString m_category;
    QWidget* m_marker{nullptr};
    QLabel* m_icon{nullptr};
    std::optional<Icons::Name> m_iconName;
    QLabel* m_label{nullptr};
    QLabel* m_count{nullptr};
    bool m_active{false};
};

LibrarySidebar::LibrarySidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("categoryList");
    setFixedWidth(206);
    // Translucent layer, no panel colour of its own: navigation should not compete
    // with the covers.
    setStyleSheet(QString("LibrarySidebar { background: %1; border-radius: 10px; }")
                      .arg(Theme::rgba(Theme::surfacePanel())));
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 10, 8, 10);
    m_layout->setSpacing(2);
    addSection("LIBRARY");
    addCategory("All Games", "Todos", Icons::Name::Grid);
    addCategory("Recently Played", "Recientes", Icons::Name::Clock);
    m_layout->addSpacing(12);
    addSection("SYSTEMS");
    addCategory("Game Boy", "GB");
    addCategory("Game Boy Color", "GBC");
    addCategory("Game Boy Advance", "GBA");
    addCategory("Nintendo DS", "NDS");
    m_layout->addStretch();
    selectCategory(m_currentCategory);
}

void LibrarySidebar::setCounts(const QHash<QString, int>& counts)
{
    for (auto it = m_rows.cbegin(); it != m_rows.cend(); ++it) {
        static_cast<CategoryRow*>(it.value())->setCount(counts.value(it.key()));
    }
}

QString LibrarySidebar::currentCategory() const
{
    return m_currentCategory;
}

void LibrarySidebar::setCurrentCategory(const QString& category)
{
    selectCategory(category);
}

void LibrarySidebar::addSection(const QString& title)
{
    auto* heading = new QLabel(title, this);
    QFont font = heading->font();
    font.setCapitalization(QFont::AllUppercase);
    font.setPixelSize(11);
    font.setWeight(QFont::DemiBold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    heading->setFont(font);
    heading->setStyleSheet(QString("color: %1; background: transparent; padding: 6px 4px 2px 4px;")
                               .arg(Theme::textDisabled().name()));
    m_layout->addWidget(heading);
}

void LibrarySidebar::addCategory(const QString& label, const QString& category, std::optional<Icons::Name> icon)
{
    auto* row = new CategoryRow(label, category, this, icon);
    m_rows.insert(category, row);
    connect(row, &CategoryRow::activated, this, &LibrarySidebar::selectCategory);
    connect(row, &CategoryRow::moveRequested, this, &LibrarySidebar::moveCategory);
    m_layout->addWidget(row);
}

void LibrarySidebar::moveCategory(int offset)
{
    const QStringList categories{"Todos", "Recientes", "GB", "GBC", "GBA", "NDS"};
    const int current = categories.indexOf(m_currentCategory);
    const int next = qBound(0, current + offset, categories.size() - 1);
    if (next == current) {
        return;
    }
    selectCategory(categories.at(next));
    m_rows.value(m_currentCategory)->setFocus(Qt::OtherFocusReason);
}

void LibrarySidebar::selectCategory(const QString& category)
{
    if (!m_rows.contains(category)) {
        return;
    }
    const bool changed = m_currentCategory != category;
    m_currentCategory = category;
    for (auto it = m_rows.cbegin(); it != m_rows.cend(); ++it) {
        static_cast<CategoryRow*>(it.value())->setActive(it.key() == category);
    }
    if (changed) {
        emit categoryChanged(category);
    }
}

} // namespace Pocket::App

#include "LibrarySidebar.moc"
