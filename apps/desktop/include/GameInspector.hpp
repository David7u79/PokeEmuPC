#pragma once

#include <QWidget>

#include "pocket/core/Game.hpp"

class QLabel;
class QPushButton;
class QStackedLayout;
class QToolButton;

namespace Pocket::App {

class InspectorCoverWidget;

class GameInspector : public QWidget {
    Q_OBJECT

public:
    explicit GameInspector(QWidget* parent = nullptr);

    void setGame(const Core::Game& game, const QString& artworkPath);
    void clear();

signals:
    void playRequested();
    void changeArtworkRequested();
    void chooseImageRequested();
    void openLocationRequested();
    void removeRequested();

private:
    QWidget* m_content{nullptr};
    QStackedLayout* m_stack{nullptr};
    QLabel* m_title{nullptr};
    QLabel* m_platform{nullptr};
    QLabel* m_saveValue{nullptr};
    QLabel* m_addedValue{nullptr};
    QLabel* m_sizeValue{nullptr};
    QLabel* m_romPath{nullptr};
    QLabel* m_sha256{nullptr};
    QPushButton* m_playButton{nullptr};
    QPushButton* m_moreActionsButton{nullptr};
    QToolButton* m_detailsButton{nullptr};
    InspectorCoverWidget* m_cover{nullptr};
};

} // namespace Pocket::App
