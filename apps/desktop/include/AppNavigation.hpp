#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>

namespace Pocket::App {

class AppNavigation : public QWidget
{
    Q_OBJECT

public:
    enum class Section {
        Library,
        Companion,
        Settings
    };
    Q_ENUM(Section)

    explicit AppNavigation(QWidget* parent = nullptr);

    Section activeSection() const;
    QPushButton* libraryButton() const { return m_libraryButton; }
    QPushButton* companionButton() const { return m_companionButton; }
    QPushButton* settingsButton() const { return m_settingsButton; }
    QPushButton* resumeButton() const { return m_resumeButton; }

public slots:
    void setActiveSection(Section section);
    // Shown only while a game is loaded: leaving the emulator is one click, and so
    // is coming back to it.
    void setRunningGame(const QString& title);
    void clearRunningGame();

signals:
    void sectionSelected(Section section);
    void resumeRequested();

private:
    void updateButtonStates();

    Section m_activeSection{Section::Library};
    QLabel* m_brandLabel{nullptr};
    QPushButton* m_libraryButton{nullptr};
    QPushButton* m_companionButton{nullptr};
    QPushButton* m_settingsButton{nullptr};
    QPushButton* m_resumeButton{nullptr};
};

} // namespace Pocket::App
