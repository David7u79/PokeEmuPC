#pragma once

#include <QHash>
#include <QWidget>

#include <optional>

#include "Icons.hpp"

class QVBoxLayout;

namespace Pocket::App {

class LibrarySidebar : public QWidget {
    Q_OBJECT

public:
    explicit LibrarySidebar(QWidget* parent = nullptr);

    void setCounts(const QHash<QString, int>& counts);
    QString currentCategory() const;
    void setCurrentCategory(const QString& category);

signals:
    void categoryChanged(const QString& category);

private:
    void addSection(const QString& title);
    void addCategory(const QString& label, const QString& category, std::optional<Icons::Name> icon = std::nullopt);
    void selectCategory(const QString& category);
    void moveCategory(int offset);

    QVBoxLayout* m_layout{nullptr};
    QHash<QString, QWidget*> m_rows;
    QString m_currentCategory{"Todos"};
};

} // namespace Pocket::App
