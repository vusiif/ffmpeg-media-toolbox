#include "JoinPage.h"

#include "i18n/TrId.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QStringListModel>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

JoinPage::JoinPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                   JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *body = new QHBoxLayout();
    body->setSpacing(16);

    // Left Column: File List Card
    auto *leftCard = new ElaScrollPageArea(this);
    leftCard->setMinimumHeight(0);
    leftCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(10);

    m_listHeader = new ElaText(QString(), 15, leftCard);
    m_listHeader->setStyleSheet(QStringLiteral("font-weight: bold;"));
    leftLayout->addWidget(m_listHeader);

    m_fileView = new ElaListView(leftCard);
    m_fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileView->setItemHeight(32);
    m_fileView->setIsTransparent(true);
    m_fileModel = new QStringListModel(this);
    m_fileView->setModel(m_fileModel);
    leftLayout->addWidget(m_fileView, 1);

    auto *fileButtons = new QHBoxLayout();
    m_addButton = new ElaPushButton(leftCard);
    m_removeButton = new ElaPushButton(leftCard);
    connect(m_addButton, &ElaPushButton::clicked, this, &JoinPage::addFiles);
    connect(m_removeButton, &ElaPushButton::clicked, this, &JoinPage::removeSelected);
    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_removeButton);
    fileButtons->addStretch();
    leftLayout->addLayout(fileButtons);

    body->addWidget(leftCard, 1);

    // Right Column: Settings Card
    auto *settingsCard = new ElaScrollPageArea(this);
    settingsCard->setMinimumHeight(0);
    settingsCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *form = new QFormLayout(settingsCard);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(12);

    m_modeCaption = new ElaText(QString(), 13, settingsCard);
    m_modeCombo = new ElaComboBox(settingsCard);
    m_modeCombo->addItems({QString(), QString(), QString()});
    connect(m_modeCombo, &ElaComboBox::currentTextChanged,
            this, &JoinPage::updateModeVisibility);

    m_colsCaption = new ElaText(QString(), 13, settingsCard);
    m_colsSpin = new ElaSpinBox(settingsCard);
    m_colsSpin->setRange(1, 16);
    m_colsSpin->setValue(2);

    m_spacingCaption = new ElaText(QString(), 13, settingsCard);
    m_spacingSpin = new ElaSpinBox(settingsCard);
    m_spacingSpin->setRange(0, 100);

    m_paddingCaption = new ElaText(QString(), 13, settingsCard);
    m_paddingSpin = new ElaSpinBox(settingsCard);
    m_paddingSpin->setRange(0, 100);

    m_backgroundCaption = new ElaText(QString(), 13, settingsCard);
    m_backgroundCombo = new ElaComboBox(settingsCard);
    m_backgroundCombo->addItems({QStringLiteral("black"), QStringLiteral("white"),
                                 QStringLiteral("transparent")});

    form->addRow(m_modeCaption, m_modeCombo);
    form->addRow(m_colsCaption, m_colsSpin);
    form->addRow(m_spacingCaption, m_spacingSpin);
    form->addRow(m_paddingCaption, m_paddingSpin);
    form->addRow(m_backgroundCaption, m_backgroundCombo);

    auto *rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addWidget(settingsCard);
    rightLayout->addStretch();
    body->addLayout(rightLayout, 1);

    root->addLayout(body, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_queueButton = new ElaPushButton(this);
    m_queueButton->setFixedHeight(38);
    m_queueButton->setMinimumWidth(140);
    connect(m_queueButton, &ElaPushButton::clicked, this, &JoinPage::addToQueue);
    buttonRow->addWidget(m_queueButton);
    root->addLayout(buttonRow);

    setAcceptDrops(true);
    updateModeVisibility();
    retranslateUi();
}

void JoinPage::updateModeVisibility()
{
    const bool isGrid = m_modeCombo->currentIndex() == 2;
    m_colsCaption->setVisible(isGrid);
    m_colsSpin->setVisible(isGrid);
}

void JoinPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void JoinPage::syncFileListModel()
{
    QStringList names;
    names.reserve(m_inputFiles.size());
    for (const QString &path : m_inputFiles) {
        names << QFileInfo(path).fileName();
    }
    m_fileModel->setStringList(names);
}

void JoinPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        const QString path = url.toLocalFile();
        if (path.isEmpty() || m_inputFiles.contains(path)) continue;
        m_inputFiles.append(path);
    }
    syncFileListModel();
    event->acceptProposedAction();
}

void JoinPage::addFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, trId("image.join"), QString(),
        QStringLiteral("Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif);;All files (*)"));
    for (const QString &path : paths) {
        if (!m_inputFiles.contains(path)) {
            m_inputFiles.append(path);
        }
    }
    syncFileListModel();
}

void JoinPage::removeSelected()
{
    const auto selected = m_fileView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const auto &index : selected) {
        rows.append(index.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        if (row >= 0 && row < m_inputFiles.size()) {
            m_inputFiles.removeAt(row);
        }
    }
    syncFileListModel();
}

void JoinPage::addToQueue()
{
    if (m_inputFiles.size() < 2) return;

    const QString outputPath = OutputNaming::build(m_inputFiles.first(),
                                                   QStringLiteral("{name}_joined.{format}"),
                                                   QStringLiteral("png"));

    const int mode = m_modeCombo->currentIndex();
    const int spacing = m_spacingSpin->value();
    const int padding = m_paddingSpin->value();

    QStringList args;
    if (mode == 2 || spacing > 0 || padding > 0) {
        // Grid mode or any margin/gap: route through the tested grid builder,
        // which pads each input (gaps), lays out by columns and wraps the
        // finished stack in an outer margin.
        const int cols = mode == 0 ? m_inputFiles.size()
                       : mode == 1 ? 1
                                   : m_colsSpin->value();
        args = m_builder->buildGridJoin(m_inputFiles, outputPath, cols, spacing,
                                        m_backgroundCombo->currentText(), padding);
    } else {
        // Plain stacking without gaps keeps the forgiving hstack/vstack,
        // which tolerate mixed image sizes.
        args << QStringLiteral("-y");
        for (const QString &file : m_inputFiles) {
            args << QStringLiteral("-i") << file;
        }
        const QString filter = (mode == 0)
            ? QStringLiteral("hstack=inputs=%1").arg(m_inputFiles.size())
            : QStringLiteral("vstack=inputs=%1").arg(m_inputFiles.size());
        args << QStringLiteral("-filter_complex") << filter << outputPath;
    }

    if (args.isEmpty()) return;

    auto *job = new Job();
    job->setName(trId("image.joinJob").arg(m_inputFiles.size()));
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void JoinPage::retranslateUi()
{
    m_titleLabel->setText(trId("image.join"));
    m_listHeader->setText(trId("batch.fileList"));
    m_addButton->setText(trId("common.open"));
    m_removeButton->setText(trId("common.delete"));
    m_modeCaption->setText(trId("common.mode") + QStringLiteral(":"));
    m_modeCombo->setItemText(0, trId("image.join.horizontal"));
    m_modeCombo->setItemText(1, trId("image.join.vertical"));
    m_modeCombo->setItemText(2, trId("image.join.grid"));
    m_colsCaption->setText(trId("common.cols") + QStringLiteral(":"));
    m_spacingCaption->setText(trId("common.spacing") + QStringLiteral(":"));
    m_paddingCaption->setText(trId("common.padding") + QStringLiteral(":"));
    m_backgroundCaption->setText(trId("common.background") + QStringLiteral(":"));
    m_queueButton->setText(trId("convert.addToQueue"));
}
