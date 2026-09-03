#include "ArtworkPickerDialog.hpp"

#include "ArtworkIndex.hpp"
#include "GameArtworkLoader.hpp"
#include "pocket/storage/LibretroArtworkProvider.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkReply>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString repoForSystem(const QString& system)
{
    if (system == "GB") return "Nintendo_-_Game_Boy";
    if (system == "GBC") return "Nintendo_-_Game_Boy_Color";
    if (system == "GBA") return "Nintendo_-_Game_Boy_Advance";
    if (system == "NDS") return "Nintendo_-_Nintendo_DS";
    return {};
}

QString platformForSystem(const QString& system)
{
    if (system == "GB") return "Nintendo - Game Boy";
    if (system == "GBC") return "Nintendo - Game Boy Color";
    if (system == "GBA") return "Nintendo - Game Boy Advance";
    if (system == "NDS") return "Nintendo - Nintendo DS";
    return {};
}

} // namespace

namespace Pocket::App {

ArtworkPickerDialog::ArtworkPickerDialog(const QString& gameTitle, const QString& system,
                                         GameArtworkLoader* loader, QWidget* parent)
    : QDialog(parent)
    , m_repo(repoForSystem(system))
    , m_platform(platformForSystem(system))
    , m_loader(loader)
{
    setWindowTitle("Elegir carátula");
    auto* layout = new QVBoxLayout(this);
    auto* content = new QHBoxLayout;
    auto* left = new QVBoxLayout;
    m_search = new QLineEdit(gameTitle, this);
    m_search->setObjectName("artworkSearch");
    m_candidates = new QListWidget(this);
    m_candidates->setObjectName("artworkCandidates");
    left->addWidget(m_search);
    left->addWidget(m_candidates);
    m_preview = new QLabel(this);
    m_preview->setObjectName("artworkPreview");
    m_preview->setFixedSize(200, 200);
    m_preview->setAlignment(Qt::AlignCenter);
    content->addLayout(left, 1);
    content->addWidget(m_preview);
    layout->addLayout(content);
    auto* buttons = new QDialogButtonBox(this);
    m_useButton = buttons->addButton("Usar esta", QDialogButtonBox::AcceptRole);
    QPushButton* cancelButton = buttons->addButton("Cancelar", QDialogButtonBox::RejectRole);
    m_useButton->setEnabled(false);
    layout->addWidget(buttons);

    connect(m_search, &QLineEdit::textChanged, this, &ArtworkPickerDialog::populateCandidates);
    connect(m_candidates, &QListWidget::currentTextChanged, this, [this] {
        m_useButton->setEnabled(m_candidates->currentItem() != nullptr);
        previewSelection();
    });
    connect(m_useButton, &QPushButton::clicked, this, [this] {
        m_chosenName = m_candidates->currentItem()->text();
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    if (!m_loader || m_repo.isEmpty()) {
        m_candidates->addItem("No se pudo cargar el índice");
        return;
    }
    ArtworkIndex* index = m_loader->index();
    connect(index, &ArtworkIndex::indexLoaded, this, [this](const QString& repo) {
        if (repo == m_repo) {
            populateCandidates(m_search->text());
        }
    });
    connect(index, &ArtworkIndex::indexFailed, this, [this](const QString& repo) {
        if (repo == m_repo) {
            m_candidates->clear();
            m_candidates->addItem("No se pudo cargar el índice");
        }
    });
    if (index->isLoaded(m_repo)) {
        populateCandidates(gameTitle);
    } else {
        m_candidates->addItem("Cargando índice…");
        index->ensureLoaded(m_repo);
    }
}

QString ArtworkPickerDialog::chosenName() const
{
    return m_chosenName;
}

void ArtworkPickerDialog::populateCandidates(const QString& query)
{
    if (!m_loader || !m_loader->index()->isLoaded(m_repo)) {
        return;
    }
    const QString current = m_candidates->currentItem() ? m_candidates->currentItem()->text() : QString();
    m_candidates->clear();
    m_candidates->addItems(ArtworkIndex::rankedMatches(query, m_loader->index()->names(m_repo)));
    const QList<QListWidgetItem*> matching = m_candidates->findItems(current, Qt::MatchExactly);
    if (!matching.isEmpty()) {
        m_candidates->setCurrentItem(matching.first());
    } else if (m_candidates->count() > 0) {
        m_candidates->setCurrentRow(0);
    }
}

void ArtworkPickerDialog::previewSelection()
{
    if (!m_candidates->currentItem()) {
        return;
    }
    m_currentPreviewName = m_candidates->currentItem()->text();
    m_preview->setPixmap(QPixmap());
    m_preview->setText("Cargando…");
    // Built by the provider, not by hand: the URL assembled here carried an extra
    // path segment and every preview came back 404.
    const std::string url = Storage::LibretroArtworkProvider::buildUrl(
        m_platform.toStdString(), m_currentPreviewName.toStdString(), Storage::ArtworkType::BoxArt);
    QNetworkRequest request{QUrl(QString::fromStdString(url))};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_network.get(request);
    const QString requestedName = m_currentPreviewName;
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestedName] {
        const QByteArray bytes = reply->readAll();
        const bool succeeded = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        if (requestedName != m_currentPreviewName) {
            return;
        }
        QImage image;
        if (!succeeded || !image.loadFromData(bytes)) {
            m_preview->setPixmap(QPixmap());
            m_preview->setText("Sin vista previa");
            return;
        }
        m_preview->setText({});
        m_preview->setPixmap(QPixmap::fromImage(image).scaled(m_preview->size(), Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation));
    });
}

} // namespace Pocket::App
