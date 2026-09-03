#include "AppNavigation.hpp"
#include <QHBoxLayout>
#include <QStyle>

namespace Pocket::App {

AppNavigation::AppNavigation(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("appNavigation");
    setFixedHeight(48);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(8);

    // Discrete brand on the left
    m_brandLabel = new QLabel(QStringLiteral("PocketPartner"), this);
    m_brandLabel->setObjectName("navBrand");
    layout->addWidget(m_brandLabel);

    // Spacing between brand and center navigation tabs
    layout->addStretch();

    // Center navigation tabs
    m_libraryButton = new QPushButton(QStringLiteral("LIBRARY"), this);
    m_libraryButton->setObjectName("navLibrary");
    m_libraryButton->setFocusPolicy(Qt::StrongFocus);
    m_libraryButton->setCheckable(true);
    layout->addWidget(m_libraryButton);

    m_companionButton = new QPushButton(QStringLiteral("COMPANION"), this);
    m_companionButton->setObjectName("navCompanion");
    m_companionButton->setFocusPolicy(Qt::StrongFocus);
    m_companionButton->setCheckable(true);
    layout->addWidget(m_companionButton);

    // Spacing between navigation tabs and right actions
    layout->addStretch();

    // Back into the running game. Hidden until there is one.
    m_resumeButton = new QPushButton(this);
    m_resumeButton->setObjectName("navResume");
    m_resumeButton->setFocusPolicy(Qt::StrongFocus);
    m_resumeButton->setVisible(false);
    layout->addWidget(m_resumeButton);

    // Settings button on the right
    m_settingsButton = new QPushButton(QString::fromUtf8("⚙ Settings"), this);
    m_settingsButton->setObjectName("navSettings");
    m_settingsButton->setFocusPolicy(Qt::StrongFocus);
    m_settingsButton->setCheckable(true);
    layout->addWidget(m_settingsButton);

    connect(m_libraryButton, &QPushButton::clicked, this, [this]() {
        setActiveSection(Section::Library);
        emit sectionSelected(Section::Library);
    });

    connect(m_companionButton, &QPushButton::clicked, this, [this]() {
        setActiveSection(Section::Companion);
        emit sectionSelected(Section::Companion);
    });

    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        setActiveSection(Section::Settings);
        emit sectionSelected(Section::Settings);
    });

    connect(m_resumeButton, &QPushButton::clicked, this, &AppNavigation::resumeRequested);

    updateButtonStates();
}

void AppNavigation::setRunningGame(const QString& title)
{
    const QString shown = title.size() > 28 ? title.left(27) + QChar(0x2026) : title;
    m_resumeButton->setText(QString::fromUtf8("▶ %1").arg(shown));
    m_resumeButton->setToolTip(QStringLiteral("Volver a %1").arg(title));
    m_resumeButton->setVisible(true);
}

void AppNavigation::clearRunningGame()
{
    m_resumeButton->setVisible(false);
    m_resumeButton->setText({});
    m_resumeButton->setToolTip({});
}

AppNavigation::Section AppNavigation::activeSection() const
{
    return m_activeSection;
}

void AppNavigation::setActiveSection(Section section)
{
    m_activeSection = section;
    updateButtonStates();
}

void AppNavigation::updateButtonStates()
{
    const bool isLib = (m_activeSection == Section::Library);
    const bool isComp = (m_activeSection == Section::Companion);
    const bool isSettings = (m_activeSection == Section::Settings);

    if (m_libraryButton) {
        m_libraryButton->setChecked(isLib);
        m_libraryButton->setProperty("active", isLib);
        if (m_libraryButton->style()) {
            m_libraryButton->style()->unpolish(m_libraryButton);
            m_libraryButton->style()->polish(m_libraryButton);
        }
    }

    if (m_companionButton) {
        m_companionButton->setChecked(isComp);
        m_companionButton->setProperty("active", isComp);
        if (m_companionButton->style()) {
            m_companionButton->style()->unpolish(m_companionButton);
            m_companionButton->style()->polish(m_companionButton);
        }
    }

    if (m_settingsButton) {
        m_settingsButton->setChecked(isSettings);
        m_settingsButton->setProperty("active", isSettings);
        if (m_settingsButton->style()) {
            m_settingsButton->style()->unpolish(m_settingsButton);
            m_settingsButton->style()->polish(m_settingsButton);
        }
    }

    setProperty("activeSection", static_cast<int>(m_activeSection));
}

} // namespace Pocket::App
