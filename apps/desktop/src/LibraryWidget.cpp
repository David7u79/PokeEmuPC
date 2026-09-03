#include "LibraryWidget.hpp"
#include "GameArtworkLoader.hpp"
#include "GameCardDelegate.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QMetaObject>
#include <QPointer>
#include <QProgressDialog>
#include <QRunnable>
#include <QThreadPool>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QListView>
#include <QLineEdit>
#include <QComboBox>
#include <QMenu>
#include <atomic>

namespace { enum GameRoles { GameIdRole=Qt::UserRole+1, SystemRole, ArtworkRole, ImportedRole };
class GameProxyModel final : public QSortFilterProxyModel { public: QString system; bool recent{false}; void refresh(){beginFilterChange();endFilterChange();} protected: bool filterAcceptsRow(int row,const QModelIndex& parent)const override { const QModelIndex index=sourceModel()->index(row,0,parent); return QSortFilterProxyModel::filterAcceptsRow(row,parent)&&(system=="Todos"||index.data(SystemRole).toString()==system); } bool lessThan(const QModelIndex&a,const QModelIndex&b)const override { if(recent)return sourceModel()->data(a,ImportedRole).toLongLong()>sourceModel()->data(b,ImportedRole).toLongLong(); return QString::localeAwareCompare(sourceModel()->data(a).toString(),sourceModel()->data(b).toString())<0; } }; }
namespace Pocket::App {
LibraryWidget::LibraryWidget(std::shared_ptr<Storage::GameRepository> repo,QWidget* parent):QWidget(parent),m_repo(std::move(repo)) {
 auto *main=new QVBoxLayout(this); auto *header=new QHBoxLayout; auto *title=new QLabel("Biblioteca",this); QFont f=title->font();f.setBold(true);f.setPointSize(f.pointSize()+2);title->setFont(f);
 m_search=new QLineEdit(this);m_search->setObjectName("librarySearch");m_search->setPlaceholderText("Buscar…");m_search->setClearButtonEnabled(true);
 m_systemFilter=new QComboBox(this);m_systemFilter->setObjectName("systemFilter");m_systemFilter->addItems({"Todos","GB","GBC","GBA","NDS"});m_sortOrder=new QComboBox(this);m_sortOrder->setObjectName("sortOrder");m_sortOrder->addItems({"Título (A-Z)","Añadido recientemente"});m_addButton=new QPushButton("Añadir juego",this);m_addButton->setObjectName("addGameButton");
 header->addWidget(title);header->addWidget(m_search);header->addWidget(m_systemFilter);header->addWidget(m_sortOrder);header->addStretch();header->addWidget(m_addButton);main->addLayout(header);
 m_model=new QStandardItemModel(this);m_proxy=new GameProxyModel; m_proxy->setParent(this);m_proxy->setSourceModel(m_model);m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);m_proxy->setFilterKeyColumn(0);
 m_grid=new QListView(this);m_grid->setObjectName("gameGrid");m_grid->setViewMode(QListView::IconMode);m_grid->setResizeMode(QListView::Adjust);m_grid->setMovement(QListView::Static);m_grid->setUniformItemSizes(true);m_grid->setSpacing(12);m_grid->setSelectionMode(QAbstractItemView::SingleSelection);m_grid->setWordWrap(true);m_grid->setModel(m_proxy);m_grid->setItemDelegate(new GameCardDelegate(m_grid));main->addWidget(m_grid);
 m_emptyLabel=new QLabel("Todavía no hay juegos. Pulsa «Añadir juego».",this);m_emptyLabel->setAlignment(Qt::AlignCenter);main->addWidget(m_emptyLabel);m_statusLabel=new QLabel(this);m_statusLabel->setObjectName("libraryStatus");main->addWidget(m_statusLabel);m_artworkLoader=new GameArtworkLoader(this);
 connect(m_addButton,&QPushButton::clicked,this,&LibraryWidget::onAddGameClicked);connect(m_search,&QLineEdit::textChanged,this,[this](const QString&t){m_proxy->setFilterFixedString(t);updateStatus();});connect(m_systemFilter,&QComboBox::currentTextChanged,this,[this]{applyFilters();});connect(m_sortOrder,&QComboBox::currentIndexChanged,this,[this]{applyFilters();});connect(m_grid,&QListView::activated,this,&LibraryWidget::playGame);connect(m_grid,&QListView::doubleClicked,this,&LibraryWidget::playGame);connect(m_artworkLoader,&GameArtworkLoader::artworkReady,this,[this](const QString&id,const QString&path){for(int r=0;r<m_model->rowCount();++r)if(m_model->index(r,0).data(GameIdRole).toString()==id)m_model->setData(m_model->index(r,0),path,ArtworkRole);});
 m_grid->setContextMenuPolicy(Qt::CustomContextMenu);connect(m_grid,&QListView::customContextMenuRequested,this,[this](const QPoint&p){const QModelIndex i=m_grid->indexAt(p);if(!i.isValid())return;QMenu menu(this);auto*play=menu.addAction("Jugar");connect(play,&QAction::triggered,this,[this,i]{playGame(i);});menu.exec(m_grid->viewport()->mapToGlobal(p));});refreshLibrary();
}
void LibraryWidget::applyFilters(){auto*p=static_cast<GameProxyModel*>(m_proxy);p->system=m_systemFilter->currentText();p->recent=m_sortOrder->currentIndex()==1;p->refresh();m_proxy->sort(0);updateStatus();}
void LibraryWidget::updateEmptyState(){const bool empty=m_model->rowCount()==0;m_emptyLabel->setVisible(empty);m_grid->setVisible(!empty);}
void LibraryWidget::updateStatus(){const int total=m_model->rowCount(),shown=m_proxy->rowCount();m_statusLabel->setText(shown==total?QString("%1 juegos").arg(total):QString("%1 de %2 juegos").arg(shown).arg(total));}
void LibraryWidget::refreshLibrary(){if(!m_repo)return;m_model->clear();for(const auto&g:m_repo->getAllGames()){auto*item=new QStandardItem(QString::fromStdString(g.title));const QString id=QString::fromStdString(g.id.toString());const QString sys=QString::fromStdString(Core::GameSystemUtils::toString(g.system));item->setData(id,GameIdRole);item->setData(sys,SystemRole);item->setData(QString(),ArtworkRole);item->setData(qint64(g.importedAtTs),ImportedRole);m_model->appendRow(item);m_artworkLoader->requestArtwork(id,item->text(),sys,QString::fromStdString(g.romPath));}applyFilters();updateEmptyState();updateStatus();}
void LibraryWidget::playGame(const QModelIndex&index){if(!index.isValid()||!m_repo)return;const QString id=index.data(GameIdRole).toString();for(const auto& game:m_repo->getAllGames())if(QString::fromStdString(game.id.toString())==id){emit gameSelected(game);return;}}
void LibraryWidget::onAddGameClicked(){QString filePath=QFileDialog::getOpenFileName(this,"Import Game ROM","","Supported ROMs (*.gb *.gbc *.gba *.nds);;Game Boy (*.gb);;Game Boy Color (*.gbc);;Game Boy Advance (*.gba);;Nintendo DS (*.nds);;All Files (*.*)");if(filePath.isEmpty()||!m_repo)return;const std::string romFilePath=filePath.toStdString();if(m_repo->isPathAlreadyImported(romFilePath)){QMessageBox::warning(this,"Import Failed","Game path has already been imported into the library.");return;}auto*progressDialog=new QProgressDialog(QString("Importing %1...").arg(QFileInfo(filePath).fileName()),"Cancel",0,100,this);progressDialog->setWindowModality(Qt::WindowModal);progressDialog->setMinimumDuration(300);progressDialog->setAutoClose(false);progressDialog->setAutoReset(false);progressDialog->show();m_addButton->setEnabled(false);auto cancelled=std::make_shared<std::atomic_bool>(false);connect(progressDialog,&QProgressDialog::canceled,this,[cancelled]{cancelled->store(true);});QPointer<QProgressDialog>dialog(progressDialog);QPointer<LibraryWidget>widget(this);auto*task=QRunnable::create([widget,romFilePath,cancelled,dialog]{const Core::RomFingerprint fingerprint=Core::RomFingerprint::calculate(romFilePath,[widget,cancelled,dialog](qint64 done,qint64 total){if(cancelled->load())return false;const int percent=total>0?static_cast<int>((done*100)/total):100;if(!widget)return false;QMetaObject::invokeMethod(widget,[dialog,percent]{if(dialog)dialog->setValue(percent);},Qt::QueuedConnection);return true;});if(!widget)return;QMetaObject::invokeMethod(widget,[widget,romFilePath,cancelled,dialog,fingerprint]{if(dialog){dialog->close();dialog->deleteLater();}widget->m_addButton->setEnabled(true);if(cancelled->load())return;if(!fingerprint.isValid()){QMessageBox::warning(widget,"Import Failed","Unable to compute ROM fingerprint.");return;}const auto result=widget->m_repo->importGame(romFilePath,fingerprint);if(result.status==Storage::ImportResultStatus::Success){widget->refreshLibrary();QMessageBox::information(widget,"Import Successful",QString("Successfully imported %1.").arg(QString::fromStdString(result.game->title)));}else QMessageBox::warning(widget,"Import Failed",QString::fromStdString(result.errorMessage));},Qt::QueuedConnection);});QThreadPool::globalInstance()->start(task);}
}
