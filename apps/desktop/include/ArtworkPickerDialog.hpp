#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace Pocket::App {

class GameArtworkLoader;

class ArtworkPickerDialog : public QDialog {
    Q_OBJECT

public:
    ArtworkPickerDialog(const QString& gameTitle, const QString& system,
                        GameArtworkLoader* loader, QWidget* parent = nullptr);

    QString chosenName() const;

private:
    void populateCandidates(const QString& query);
    void previewSelection();

    QString m_repo;
    QString m_platform;
    GameArtworkLoader* m_loader{nullptr};
    QLineEdit* m_search{nullptr};
    QListWidget* m_candidates{nullptr};
    QLabel* m_preview{nullptr};
    QPushButton* m_useButton{nullptr};
    QNetworkAccessManager m_network;
    QString m_currentPreviewName;
    QString m_chosenName;
};

} // namespace Pocket::App
