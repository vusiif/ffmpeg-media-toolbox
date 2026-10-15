#include "ImagePage.h"

#include <QtConcurrent/QtConcurrentRun>

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

ImagePage::ImagePage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(16);

    auto *titleRow = new QHBoxLayout();
    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    m_browseButton = new ElaPushButton(this);
    m_browseButton->setFixedHeight(32);
    connect(m_browseButton, &ElaPushButton::clicked, this, &ImagePage::browseInput);
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();
    titleRow->addWidget(m_browseButton);
    root->addLayout(titleRow);

    auto *body = new QHBoxLayout();
    body->setSpacing(16);

    // Left Column: Preview Card
    auto *previewCard = new ElaScrollPageArea(this);
    previewCard->setMinimumHeight(0);
    previewCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *previewLayout = new QVBoxLayout(previewCard);
    previewLayout->setContentsMargins(16, 16, 16, 16);
    previewLayout->setSpacing(10);

    m_previewLabel = new QLabel(previewCard);
    m_previewLabel->setMinimumSize(300, 220);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(QStringLiteral("QLabel { background: rgba(128, 128, 128, 0.08); border-radius: 6px; }"));
    m_previewLabel->setScaledContents(false);

    m_fileLabel = new ElaText(QString(), 13, previewCard);
    m_fileLabel->setTextPixelSize(12);
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    previewLayout->addWidget(m_previewLabel, 1);
    previewLayout->addWidget(m_fileLabel);
    body->addWidget(previewCard, 1);

    // Right Column: Settings Card
    auto *settingsCard = new ElaScrollPageArea(this);
    settingsCard->setMinimumHeight(0);
    settingsCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *form = new QFormLayout(settingsCard);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(12);

    m_formatCaption = new ElaText(QString(), 13, settingsCard);
    m_formatCombo = new ElaComboBox(settingsCard);
    m_formatCombo->addItems({QStringLiteral("png"), QStringLiteral("jpeg"), QStringLiteral("webp"),
                             QStringLiteral("bmp")});

    m_cropCaption = new ElaText(QString(), 13, settingsCard);
    auto *cropRow = new QHBoxLayout();
    m_cropX = new ElaSpinBox(settingsCard);
    m_cropY = new ElaSpinBox(settingsCard);
    m_cropW = new ElaSpinBox(settingsCard);
    m_cropH = new ElaSpinBox(settingsCard);
    for (ElaSpinBox *spin : {m_cropX, m_cropY, m_cropW, m_cropH}) {
        spin->setRange(0, 10000);
        cropRow->addWidget(spin);
    }

    m_resizeCaption = new ElaText(QString(), 13, settingsCard);
    auto *resizeRow = new QHBoxLayout();
    m_resizeW = new ElaSpinBox(settingsCard);
    m_resizeH = new ElaSpinBox(settingsCard);
    for (ElaSpinBox *spin : {m_resizeW, m_resizeH}) {
        spin->setRange(0, 10000);
        resizeRow->addWidget(spin);
    }

    m_rotateCaption = new ElaText(QString(), 13, settingsCard);
    m_rotateCombo = new ElaComboBox(settingsCard);
    m_rotateCombo->addItems({QStringLiteral("0"), QStringLiteral("90"),
                             QStringLiteral("180"), QStringLiteral("270")});

    m_flipCaption = new ElaText(QString(), 13, settingsCard);
    auto *flipRow = new QHBoxLayout();
    m_flipH = new ElaCheckBox(settingsCard);
    m_flipV = new ElaCheckBox(settingsCard);
    flipRow->addWidget(m_flipH);
    flipRow->addWidget(m_flipV);
    flipRow->addStretch();

    m_qualityCaption = new ElaText(QString(), 13, settingsCard);
    m_qualitySpin = new ElaSpinBox(settingsCard);
    m_qualitySpin->setRange(1, 100);
    m_qualitySpin->setValue(90);

    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_cropCaption, cropRow);
    form->addRow(m_resizeCaption, resizeRow);
    form->addRow(m_rotateCaption, m_rotateCombo);
    form->addRow(m_flipCaption, flipRow);
    form->addRow(m_qualityCaption, m_qualitySpin);

    auto *rightWrapper = new QVBoxLayout();
    rightWrapper->setContentsMargins(0, 0, 0, 0);
    rightWrapper->setSpacing(10);
    rightWrapper->addWidget(settingsCard);
    rightWrapper->addStretch();
    body->addLayout(rightWrapper, 1);

    root->addLayout(body, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_addButton = new ElaPushButton(this);
    m_addButton->setFixedHeight(38);
    m_addButton->setMinimumWidth(140);
    connect(m_addButton, &ElaPushButton::clicked, this, &ImagePage::addToQueue);
    buttonRow->addWidget(m_addButton);
    root->addLayout(buttonRow);

    m_previewWatcher = new QFutureWatcher<QPixmap>(this);
    connect(m_previewWatcher, &QFutureWatcher<QPixmap>::finished, this, [=]() {
        const QPixmap pix = m_previewWatcher->result();
        if (!pix.isNull()) {
            m_previewLabel->setPixmap(pix);
        }
    });

    retranslateUi();
}

void ImagePage::browseInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("image.title"), QString(),
        QStringLiteral("Image files (*.png *.jpg *.jpeg *.webp *.bmp *.tiff *.gif);;All files (*)"));
    if (path.isEmpty()) return;

    m_inputFile = path;
    m_fileLabel->setText(QFileInfo(path).fileName());
    startPreview();
}

void ImagePage::startPreview()
{
    const QString path = m_inputFile;
    const QSize target = m_previewLabel->size();
    m_previewWatcher->setFuture(QtConcurrent::run([path, target]() {
        QPixmap pix(path);
        if (pix.isNull()) return QPixmap();
        return pix.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }));
}

void ImagePage::addToQueue()
{
    if (m_inputFile.isEmpty()) return;

    QString ext = m_formatCombo->currentText();
    if (ext == QLatin1String("jpeg")) ext = QStringLiteral("jpg");

    const QString outputPath = OutputNaming::build(m_inputFile,
                                                   QStringLiteral("{name}.{format}"), ext);

    const QStringList args = m_builder->buildImageOperation(
        m_inputFile, outputPath,
        m_cropX->value(), m_cropY->value(), m_cropW->value(), m_cropH->value(),
        m_resizeW->value(), m_resizeH->value(),
        m_rotateCombo->currentText().toInt(),
        m_flipH->isChecked(), m_flipV->isChecked(),
        m_formatCombo->currentText(),
        m_qualitySpin->value());

    auto *job = new Job();
    job->setName(trId("image.job").arg(QFileInfo(m_inputFile).fileName()));
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void ImagePage::retranslateUi()
{
    m_titleLabel->setText(trId("image.title"));
    m_browseButton->setText(trId("common.browse"));
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_cropCaption->setText(trId("image.crop") + QStringLiteral(":"));
    m_resizeCaption->setText(trId("image.resize") + QStringLiteral(":"));
    m_rotateCaption->setText(trId("image.rotate") + QStringLiteral(":"));
    m_flipCaption->setText(trId("image.flip") + QStringLiteral(":"));
    m_flipH->setText(trId("image.flipH"));
    m_flipV->setText(trId("image.flipV"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_addButton->setText(trId("convert.addToQueue"));
}
