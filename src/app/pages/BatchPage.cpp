#include "BatchPage.h"

#include "i18n/TrId.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"

BatchPage::BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
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
    m_clearButton = new ElaPushButton(this);
    connect(m_addButton, &ElaPushButton::clicked, this, &BatchPage::addFiles);
    connect(m_clearButton, &ElaPushButton::clicked, this, &BatchPage::clearFiles);
    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_clearButton);
    fileButtons->addStretch();
    left->addWidget(m_fileList, 1);
    left->addLayout(fileButtons);
    body->addLayout(left, 1);

    auto *right = new QVBoxLayout();
    right->setSpacing(10);

    m_countLabel = new ElaText(QString(), 13, this);
    m_countLabel->setTextPixelSize(12);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_outputDirCaption = new ElaText(QString(), 13, this);
    m_outputDirButton = new ElaPushButton(this);
    connect(m_outputDirButton, &ElaPushButton::clicked, this, &BatchPage::browseOutputDir);

    m_formatCaption = new ElaText(QString(), 13, this);
    m_formatCombo = new ElaComboBox(this);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi")});

    m_codecCaption = new ElaText(QString(), 13, this);
    m_codecCombo = new ElaComboBox(this);
    m_codecCombo->addItems({QStringLiteral(""), QStringLiteral("copy"), QStringLiteral("libx264"),
                            QStringLiteral("libx265"), QStringLiteral("libvpx-vp9"),
                            QStringLiteral("h264_nvenc"), QStringLiteral("hevc_nvenc")});

    m_qualityCaption = new ElaText(QString(), 13, this);
    m_qualitySpin = new ElaSpinBox(this);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);

    m_smartCopyCheck = new ElaCheckBox(this);

    form->addRow(m_outputDirCaption, m_outputDirButton);
    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_codecCaption, m_codecCombo);
    form->addRow(m_qualityCaption, m_qualitySpin);
    form->addRow(new ElaText(QString(), 13, this), m_smartCopyCheck);

    right->addWidget(m_countLabel);
    right->addLayout(form);
    right->addStretch();
    body->addLayout(right, 1);

    root->addLayout(body, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_queueButton = new ElaPushButton(this);
    connect(m_queueButton, &ElaPushButton::clicked, this, &BatchPage::addAllToQueue);
    buttonRow->addWidget(m_queueButton);
    root->addLayout(buttonRow);

    retranslateUi();
}

void BatchPage::addFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, trId("nav.queue"), QString(), QStringLiteral("All files (*)"));
    for (const QString &path : paths) {
        m_inputFiles.append(path);
        m_fileList->addItem(QFileInfo(path).fileName());
    }
    retranslateUi();
}

void BatchPage::clearFiles()
{
    m_inputFiles.clear();
    m_fileList->clear();
    retranslateUi();
}

void BatchPage::browseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("common.save"));
    if (dir.isEmpty()) return;
    m_outputDirectory = dir;
    m_outputDirButton->setText(dir);
}

void BatchPage::addAllToQueue()
{
    for (const QString &inputFile : m_inputFiles) {
        QString outputPath;
        const QFileInfo info(inputFile);
        if (!m_outputDirectory.isEmpty()) {
            outputPath = m_outputDirectory + QStringLiteral("/") + info.completeBaseName()
                         + QStringLiteral(".") + m_formatCombo->currentText();
        } else {
            outputPath = inputFile;
            outputPath.replace(QRegularExpression(QStringLiteral("\\.[^.]+$")),
                               QStringLiteral(".") + m_formatCombo->currentText());
        }

        const QStringList args = m_builder->buildConversion(
            inputFile, outputPath,
            m_formatCombo->currentText(),
            m_codecCombo->currentText(),
            QString(),
            m_qualitySpin->value(),
            m_smartCopyCheck->isChecked());

        auto *job = new Job();
        job->setName(info.fileName());
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    }
}

void BatchPage::retranslateUi()
{
    m_titleLabel->setText(trId("media.title"));
    m_addButton->setText(trId("common.open"));
    m_clearButton->setText(trId("common.clear"));
    m_countLabel->setText(QStringLiteral("%1").arg(m_inputFiles.size()));
    m_outputDirCaption->setText(trId("common.save") + QStringLiteral(":"));
    if (m_outputDirectory.isEmpty()) {
        m_outputDirButton->setText(trId("common.browse"));
    }
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_queueButton->setText(trId("convert.addToQueue"));
}
