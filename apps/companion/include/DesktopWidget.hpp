#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPoint>
#include <QProgressBar>
#include <QPixmap>
#include <memory>
#include "pocket/companion/CompanionState.hpp"
#include "pocket/companion/CompanionSimulator.hpp"
#include "pocket/companion/IClock.hpp"
#include "pocket/companion/CompanionLink.hpp"
#include "pocket/companion/SpriteKey.hpp"
#include "pocket/companion/CompositeSpriteProvider.hpp"
#include "pocket/companion/PokeSpriteProvider.hpp"
#include "pocket/companion/PkhexSpriteProvider.hpp"
#include "pocket/companion/PlaceholderSpriteProvider.hpp"
#include "pocket/companion/SpriteCache.hpp"
#include "pocket/core/IpcClient.hpp"
#include "CompanionAnimationController.hpp"
#include "PowerStatusMonitor.hpp"

namespace Pocket::Companion {

class CompanionVisualCanvas : public QWidget {
    Q_OBJECT
public:
    explicit CompanionVisualCanvas(QWidget *parent = nullptr);
    void setAnimationFrame(int frame) { m_animFrame = frame; update(); }
    void setPixmap(const QPixmap& pixmap) { m_pixmap = pixmap; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_animFrame{0};
    QPixmap m_pixmap;
};

class DesktopWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget() override;

    void updateCanonicalInfo(const QString& nickname, const QString& species, int level, int friendship, const QString& linkStatus);
    void updateCreatureSprite(uint16_t speciesId, bool shiny = false, uint8_t formId = 0, Gender gender = Gender::Unknown);

    SpriteCache& spriteCache() { return m_spriteCache; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onFeedClicked();
    void onPetClicked();
    void onRestClicked();
    void onTrainClicked();
    void onOpenGameClicked();
    void toggleExpandedView();
    void refreshUi();
    void onFrameTick();
    void onIpcMessageReceived(const Pocket::Core::IpcMessage& message);

private:
    void loadPositionSettings();
    void savePositionSettings();
    uint16_t speciesNameToId(const QString& speciesName) const;

    std::shared_ptr<IClock> m_clock;
    CompanionSimulator m_simulator;
    CompanionState m_state;
    CompanionApp::CompanionAnimationController m_animController;
    CompanionApp::PowerStatusMonitor m_powerMonitor;
    Pocket::Core::IpcClient m_ipcClient;

    CompositeSpriteProvider m_spriteProvider;
    SpriteCache m_spriteCache{32};
    SpriteKey m_currentKey{25, false, 0, Gender::Unknown};

    CompanionVisualCanvas *m_canvas{nullptr};
    QLabel *m_nicknameLabel{nullptr};
    QLabel *m_canonicalMetaLabel{nullptr};
    QLabel *m_linkStatusLabel{nullptr};

    QProgressBar *m_bondBar{nullptr};
    QProgressBar *m_energyBar{nullptr};

    QPushButton *m_feedBtn{nullptr};
    QPushButton *m_petBtn{nullptr};
    QPushButton *m_playBtn{nullptr};
    QPushButton *m_restBtn{nullptr};

    QWidget *m_expandedContainer{nullptr};
    QLabel *m_hungerLabel{nullptr};
    QLabel *m_moodLabel{nullptr};
    QLabel *m_energyDetailLabel{nullptr};
    QLabel *m_bondDetailLabel{nullptr};
    QPushButton *m_openGameBtn{nullptr};

    QPoint m_dragPosition;
    bool m_isDragging{false};
    bool m_isExpanded{false};

    int m_gameFriendship{70};
    int m_animTickCount{0};
};

} // namespace Pocket::Companion
