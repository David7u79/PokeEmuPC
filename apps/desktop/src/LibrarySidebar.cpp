#include "LibrarySidebar.hpp"

#include "Theme.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

namespace Pocket::App {

class CategoryRow final : public QWidget {
    Q_OBJECT

public:
    CategoryRow(const QString& label, const QString& category, QWidget* parent)
        : QWidget(parent)
        , m_category(category)
    {
        setFixedHeight(32);
        setFocusPolicy(Qt::StrongFocus);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 8, 0);
        layout->setSpacing(8);
        m_marker = new QWidget(this);
        m_marker->setFixedWidth(3);
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
        const QColor surface = Theme::surface();
        const QColor raised = Theme::surfaceRaised();
        const QColor accent = Theme::accent();
        const QColor primary = Theme::textPrimary();
        const QColor secondary = Theme::textSecondary();
        setStyleSheet(QString("CategoryRow { background: %1; border: 1px solid transparent; border-radius: 4px; }"
                              "CategoryRow:focus { border-color: %2; }"
                              "QLabel { background: transparent; color: %3; }"
                              "QWidget#marker { background: %4; }")
                          .arg(m_active ? raised.name() : surface.name(), accent.name(),
                               m_active ? primary.name() : secondary.name(),
                               m_active ? accent.name() : surface.name()));
        m_marker->setObjectName("marker");
    }

    QString m_category;
    QWidget* m_marker{nullptr};
    QLabel* m_label{nullptr};
    QLabel* m_count{nullptr};
    bool m_active{false};
};

LibrarySidebar::LibrarySidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("categoryList");
    setFixedWidth(210);
    setStyleSheet(QString("LibrarySidebar { background: %1; }").arg(Theme::surface().name()));
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(2);
    addSection("LIBRARY");
    addCategory("All Games", "Todos");
    addCategory("Recently Played", "Recientes");
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
    font.setPointSize(qMax(8, font.pointSize() - 2));
    font.setBold(true);
    heading->setFont(font);
    heading->setStyleSheet(QString("color: %1; background: transparent;").arg(Theme::textSecondary().name()));
    m_layout->addWidget(heading);
}

void LibrarySidebar::addCategory(const QString& label, const QString& category)
{
    auto* row = new CategoryRow(label, category, this);
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
