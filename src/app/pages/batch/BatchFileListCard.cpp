#include "BatchFileListCard.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QSettings>
#include <QStringListModel>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaLineEdit.h"
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "app/AppSettings.h"
#include "i18n/TrId.h"
#include "batch/BatchExportMatrix.h"
#include "batch/FolderWatcherService.h"

namespace {
QString defaultOutputDirectory()
{
    QSettings s(AppSettings::organization(), AppSettings::application());
    return s.value(QStringLiteral("output/directory")).toString();
}
} // namespace

BatchFileListCard::BatchFileListCard(QWidget *parent)
    : ElaScrollPageArea(parent)
    , m_watcherService(new FolderWatcherService(this))
{
    m_outputDirectory = defaultOutputDirectory();

    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *leftLayout = new QVBoxLayout(this);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(10);

    auto *listTopRow = new QHBoxLayout();
    m_listHeader = new ElaText(QString(), 14, this);
    m_listHeader->setIsWrapAnywhere(false);
    m_countLabel = new ElaText(QString(), 12, this);
    listTopRow->addWidget(m_listHeader);
    listTopRow->addStretch();
    listTopRow->addWidget(m_countLabel);
    leftLayout->addLayout(listTopRow);

    m_fileView = new ElaListView(this);
    m_fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileView->setItemHeight(32);
    m_fileView->setIsTransparent(true);
    m_fileView->installEventFilter(this);
    m_fileModel = new QStringListModel(this);
    m_fileView->setModel(m_fileModel);
    leftLayout->addWidget(m_fileView, 1);
    setAcceptDrops(true);

    auto *fileButtons = new QHBoxLayout();
    m_addButton = new ElaPushButton(this);
    m_addFolderButton = new ElaPushButton(this);
    m_removeButton = new ElaPushButton(this);
    m_clearButton = new ElaPushButton(this);
    connect(m_addButton, &ElaPushButton::clicked, this, &BatchFileListCard::onAddFilesClicked);
    connect(m_addFolderButton, &ElaPushButton::clicked, this, &BatchFileListCard::onAddFolderClicked);
    connect(m_removeButton, &ElaPushButton::clicked, this, &BatchFileListCard::onRemoveSelectedClicked);
    connect(m_clearButton, &ElaPushButton::clicked, this, &BatchFileListCard::onClearFilesClicked);

    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_addFolderButton);
    fileButtons->addWidget(m_removeButton);
    fileButtons->addWidget(m_clearButton);
    fileButtons->addStretch();
    leftLayout->addLayout(fileButtons);

    m_preserveTreeCheck = new ElaCheckBox(trId("batch.preserveTree"), this);
    m_preserveTreeCheck->setChecked(true);
    leftLayout->addWidget(m_preserveTreeCheck);

    // Output Directory row
    auto *outDirRow = new QHBoxLayout();
    m_outputDirCaption = new ElaText(QString(), 13, this);
    m_outputDirButton = new ElaPushButton(this);
    if (!m_outputDirectory.isEmpty()) {
        m_outputDirButton->setText(m_outputDirectory);
    }
    connect(m_outputDirButton, &ElaPushButton::clicked, this, &BatchFileListCard::onBrowseOutputDir);
    outDirRow->addWidget(m_outputDirCaption);
    outDirRow->addWidget(m_outputDirButton, 1);
    leftLayout->addLayout(outDirRow);

    // Folder Watcher Section
    m_watcherToggle = new ElaCheckBox(trId("batch.watcher.title"), this);
    leftLayout->addWidget(m_watcherToggle);

    m_watcherContainer = new QWidget(this);
    auto *watcherLayout = new QVBoxLayout(m_watcherContainer);
    watcherLayout->setContentsMargins(0, 4, 0, 0);
    watcherLayout->setSpacing(6);

    auto *watchPathRow = new QHBoxLayout();
    m_watchPathEdit = new ElaLineEdit(m_watcherContainer);
    m_watchBrowseBtn = new ElaPushButton(trId("common.browse"), m_watcherContainer);
    connect(m_watchBrowseBtn, &ElaPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.watcher.selectDir"));
        if (!dir.isEmpty()) {
            m_watchPathEdit->setText(dir);
        }
    });
    watchPathRow->addWidget(m_watchPathEdit, 1);
    watchPathRow->addWidget(m_watchBrowseBtn);
    watcherLayout->addLayout(watchPathRow);

    auto *watchActionRow = new QHBoxLayout();
    m_watchActionBtn = new ElaPushButton(trId("batch.watcher.start"), m_watcherContainer);
    connect(m_watchActionBtn, &ElaPushButton::clicked, this, &BatchFileListCard::onToggleFolderWatcher);
    m_watchStatusLabel = new ElaText(trId("batch.watcher.status.idle"), 12, m_watcherContainer);
    watchActionRow->addWidget(m_watchActionBtn);
    watchActionRow->addSpacing(12);
    watchActionRow->addWidget(m_watchStatusLabel, 1);
    watcherLayout->addLayout(watchActionRow);

    m_watcherContainer->setVisible(false);
    leftLayout->addWidget(m_watcherContainer);
    connect(m_watcherToggle, &ElaCheckBox::toggled, m_watcherContainer, &QWidget::setVisible);

    connect(m_watcherService, &FolderWatcherService::fileReady,
            this, &BatchFileListCard::onWatcherFileReady);

    retranslateUi();
}

BatchFileListCard::~BatchFileListCard()
{
    if (m_watcherService && m_watcherService->isWatching()) {
        m_watcherService->stopWatching();
    }
}

bool BatchFileListCard::preserveTree() const
{
    return m_preserveTreeCheck->isChecked();
}

void BatchFileListCard::addFiles(const QStringList &files)
{
    bool added = false;
    for (const QString &f : files) {
        if (!m_inputFiles.contains(f)) {
            m_inputFiles.append(f);
            added = true;
        }
    }
    if (added) {
        syncFileListModel();
        emit fileListChanged();
    }
}

void BatchFileListCard::addFolder(const QString &dir)
{
    if (dir.isEmpty()) return;
    const auto scanned = BatchExportMatrix::scanDirectory(dir, true);
    if (scanned.isEmpty()) return;

    bool added = false;
    for (const auto &item : scanned) {
        if (!m_inputFiles.contains(item.fullPath)) {
            m_inputFiles.append(item.fullPath);
            m_relativeSubDirs.insert(item.fullPath, item.relativeSubDir);
            added = true;
        }
    }
    if (added) {
        syncFileListModel();
        emit fileListChanged();
    }
}

void BatchFileListCard::onAddFilesClicked()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, trId("batch.selectFiles"), QString(),
        QStringLiteral("Media Files (*.mp4 *.mkv *.mov *.avi *.webm *.flv *.ts *.mp3 *.wav *.flac *.aac *.m4a *.opus *.ogg);;All Files (*.*)"));
    if (files.isEmpty()) return;
    addFiles(files);
}

void BatchFileListCard::onAddFolderClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.selectFolder"), QString());
    if (dir.isEmpty()) return;
    addFolder(dir);
}

void BatchFileListCard::onRemoveSelectedClicked()
{
    const QModelIndexList selected = m_fileView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const QModelIndex &idx : selected) {
        rows.append(idx.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        if (row >= 0 && row < m_inputFiles.size()) {
            m_inputFiles.removeAt(row);
        }
    }
    syncFileListModel();
    emit fileListChanged();
}

void BatchFileListCard::onClearFilesClicked()
{
    if (m_inputFiles.isEmpty()) return;
    m_inputFiles.clear();
    m_relativeSubDirs.clear();
    syncFileListModel();
    emit fileListChanged();
}

void BatchFileListCard::onBrowseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.selectOutputDir"), m_outputDirectory);
    if (!dir.isEmpty()) {
        m_outputDirectory = dir;
        m_outputDirButton->setText(dir);
    }
}

void BatchFileListCard::onToggleFolderWatcher()
{
    if (m_watcherService->isWatching()) {
        m_watcherService->stopWatching();
        m_watchActionBtn->setText(trId("batch.watcher.start"));
        m_watchStatusLabel->setText(trId("batch.watcher.status.idle"));
    } else {
        const QString watchPath = m_watchPathEdit->text().trimmed();
        if (watchPath.isEmpty() || !QDir(watchPath).exists()) {
            QMessageBox::warning(this, trId("batch.watcher.title"), trId("batch.watcher.invalidDir"));
            return;
        }
        m_watcherService->startWatching(watchPath, m_outputDirectory);
        m_watchActionBtn->setText(trId("batch.watcher.stop"));
        m_watchStatusLabel->setText(trId("batch.watcher.status.active").arg(watchPath).arg(0));
    }
}

void BatchFileListCard::onWatcherFileReady(const QString &filePath, const QString &relativeSubDir)
{
    if (!m_inputFiles.contains(filePath)) {
        m_inputFiles.append(filePath);
        if (!relativeSubDir.isEmpty()) {
            m_relativeSubDirs.insert(filePath, relativeSubDir);
        }
        syncFileListModel();
        emit fileListChanged();
    }
    m_watchStatusLabel->setText(trId("batch.watcher.status.active")
                                    .arg(m_watcherService->watchPath())
                                    .arg(m_watcherService->processedCount()));
    emit fileReadyToProcess(filePath, relativeSubDir);
}

void BatchFileListCard::syncFileListModel()
{
    m_fileModel->setStringList(m_inputFiles);
    updateCountLabel();
}

void BatchFileListCard::updateCountLabel()
{
    m_countLabel->setText(trId("batch.fileCount").arg(m_inputFiles.size()));
}

void BatchFileListCard::retranslateUi()
{
    m_listHeader->setText(trId("batch.files"));
    m_addButton->setText(trId("batch.addFiles"));
    m_addFolderButton->setText(trId("batch.addFolder"));
    m_removeButton->setText(trId("batch.removeSelected"));
    m_clearButton->setText(trId("batch.clearAll"));
    m_preserveTreeCheck->setText(trId("batch.preserveTree"));
    m_outputDirCaption->setText(trId("batch.outputDir") + QStringLiteral(":"));
    m_watcherToggle->setText(trId("batch.watcher.title"));
    m_watchBrowseBtn->setText(trId("common.browse"));
    m_watchActionBtn->setText(m_watcherService->isWatching() ? trId("batch.watcher.stop") : trId("batch.watcher.start"));
    updateCountLabel();
}

bool BatchFileListCard::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileView && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
            onRemoveSelectedClicked();
            return true;
        }
        if (ke->matches(QKeySequence::SelectAll)) {
            m_fileView->selectAll();
            return true;
        }
    }
    return ElaScrollPageArea::eventFilter(obj, event);
}

void BatchFileListCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        event->acceptProposedAction();
        update();
    }
}

void BatchFileListCard::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    m_isDragHover = false;
    update();
}

void BatchFileListCard::dropEvent(QDropEvent *event)
{
    m_isDragHover = false;
    update();

    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString path = url.toLocalFile();
        const QFileInfo fi(path);
        if (fi.isDir()) {
            addFolder(path);
        } else {
            files.append(path);
        }
    }
    if (!files.isEmpty()) {
        addFiles(files);
    }
    event->acceptProposedAction();
}

void BatchFileListCard::paintEvent(QPaintEvent *event)
{
    ElaScrollPageArea::paintEvent(event);
    if (m_isDragHover) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 25));
        p.drawRoundedRect(rect().adjusted(4, 4, -4, -4), 8, 8);
    }
}
