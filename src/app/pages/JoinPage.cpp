#include "JoinPage.h"

#include "i18n/TrId.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMimeData>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"

JoinPage::JoinPage(FFmpegLocator *locator, JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_queue(queue)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *body = new QHBoxLayout();
    body->setSpacing(16);

    auto *left = new QVBoxLayout();
    m_fileList = new QListWidget(this);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    auto *fileButtons = new QHBoxLayout();
    m_addButton = new ElaPushButton(this);
    m_removeButton = new ElaPushButton(this);
    connect(m_addButton, &ElaPushButton::clicked, this, &JoinPage::addFiles);
    connect(m_removeButton, &ElaPushButton::clicked, this, &JoinPage::removeSelected);
    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_removeButton);
    fileButtons->addStretch();
    left->addWidget(m_fileList, 1);
    left->addLayout(fileButtons);
    body->addLayout(left, 1);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_modeCaption = new ElaText(QString(), 13, this);
    m_modeCombo = new ElaComboBox(this);
    m_modeCombo->addItems({QString(), QString(), QString()});
    connect(m_modeCombo, &ElaComboBox::currentTextChanged,
            this, &JoinPage::updateModeVisibility);

    m_rowsCaption = new ElaText(QString(), 13, this);
    m_rowsSpin = new ElaSpinBox(this);
    m_rowsSpin->setRange(1, 16);
    m_rowsSpin->setValue(2);
    m_colsCaption = new ElaText(QString(), 13, this);
    m_colsSpin = new ElaSpinBox(this);
    m_colsSpin->setRange(1, 16);
    m_colsSpin->setValue(2);

    m_spacingCaption = new ElaText(QString(), 13, this);
    m_spacingSpin = new ElaSpinBox(this);
    m_spacingSpin->setRange(0, 100);

    m_paddingCaption = new ElaText(QString(), 13, this);
    m_paddingSpin = new ElaSpinBox(this);
    m_paddingSpin->setRange(0, 100);

    m_backgroundCaption = new ElaText(QString(), 13, this);
    m_backgroundCombo = new ElaComboBox(this);
    m_backgroundCombo->addItems({QStringLiteral("black"), QStringLiteral("white"),
                                 QStringLiteral("transparent")});

    form->addRow(m_modeCaption, m_modeCombo);
    form->addRow(m_rowsCaption, m_rowsSpin);
    form->addRow(m_colsCaption, m_colsSpin);
    form->addRow(m_spacingCaption, m_spacingSpin);
    form->addRow(m_paddingCaption, m_paddingSpin);
    form->addRow(m_backgroundCaption, m_backgroundCombo);
    body->addLayout(form, 1);

    root->addLayout(body, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_queueButton = new ElaPushButton(this);
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
    m_rowsCaption->setVisible(isGrid);
    m_rowsSpin->setVisible(isGrid);
    m_colsCaption->setVisible(isGrid);
    m_colsSpin->setVisible(isGrid);
}

void JoinPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void JoinPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        const QString path = url.toLocalFile();
        if (path.isEmpty()) continue;
        m_inputFiles.append(path);
        m_fileList->addItem(QFileInfo(path).fileName());
    }
    event->acceptProposedAction();
}

void JoinPage::addFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, trId("image.join"), QString(),
        QStringLiteral("Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif);;All files (*)"));
    for (const QString &path : paths) {
        m_inputFiles.append(path);
        m_fileList->addItem(QFileInfo(path).fileName());
    }
}

void JoinPage::removeSelected()
{
    const QList<QListWidgetItem *> selected = m_fileList->selectedItems();
    for (QListWidgetItem *item : selected) {
        const int row = m_fileList->row(item);
        m_inputFiles.removeAt(row);
        delete m_fileList->takeItem(row);
    }
}

void JoinPage::addToQueue()
{
    if (m_inputFiles.size() < 2) return;

    QString outputPath = m_inputFiles.first();
    outputPath.replace(QRegularExpression(QStringLiteral("\\.[^.]+$")),
                       QStringLiteral("_joined.png"));

    QStringList args;
    args << QStringLiteral("-y");
    for (const QString &file : m_inputFiles) {
        args << QStringLiteral("-i") << file;
    }

    QString filter;
    if (m_modeCombo->currentIndex() == 0) {
        filter = QStringLiteral("hstack=inputs=%1").arg(m_inputFiles.size());
    } else {
        filter = QStringLiteral("vstack=inputs=%1").arg(m_inputFiles.size());
    }

    const int spacing = m_spacingSpin->value();
    if (spacing > 0) {
        filter = QStringLiteral("pad=iw+%1:ih+%1:%2:%2:color=%3")
                     .arg(spacing)
                     .arg(spacing / 2)
                     .arg(m_backgroundCombo->currentText());
    }

    args << QStringLiteral("-filter_complex") << filter << outputPath;

    auto *job = new Job();
    job->setName(trId("image.joinJob").arg(m_inputFiles.size()));
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void JoinPage::retranslateUi()
{
    m_titleLabel->setText(trId("image.join"));
    m_addButton->setText(trId("common.open"));
    m_removeButton->setText(trId("common.delete"));
    m_modeCaption->setText(trId("common.mode") + QStringLiteral(":"));
    m_modeCombo->setItemText(0, trId("image.join.horizontal"));
    m_modeCombo->setItemText(1, trId("image.join.vertical"));
    m_modeCombo->setItemText(2, trId("image.join.grid"));
    m_rowsCaption->setText(trId("common.rows") + QStringLiteral(":"));
    m_colsCaption->setText(trId("common.cols") + QStringLiteral(":"));
    m_spacingCaption->setText(trId("common.spacing") + QStringLiteral(":"));
    m_paddingCaption->setText(trId("common.padding") + QStringLiteral(":"));
    m_backgroundCaption->setText(trId("common.background") + QStringLiteral(":"));
    m_queueButton->setText(trId("convert.addToQueue"));
}
