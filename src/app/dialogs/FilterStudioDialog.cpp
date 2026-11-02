#include "FilterStudioDialog.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPivot.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/widgets/VideoPlayerWidget.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaFile.h"
#include "media/MediaProbeService.h"
#include "output/OutputNaming.h"

FilterStudioDialog::FilterStudioDialog(FFmpegLocator *locator, MediaProbeService *probeService,
                                       JobQueue *queue, QWidget *parent)
    : QDialog(parent)
    , m_locator(locator)
    , m_probeService(probeService)
    , m_queue(queue)
{
    setWindowTitle(trId("filter.dialog.title"));
    setMinimumSize(1040, 800);
    resize(1100, 860);
    setAcceptDrops(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setIsGrabGesture(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *body = new QWidget(scroll);
    body->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 10, 0);
    bodyLayout->setSpacing(12);

    // 1. Header Card (File picker & info)
    auto *headerCard = new ElaScrollPageArea(body);
    auto *headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(8);

    m_titleLabel = new ElaText(trId("filter.dialog.title"), 16, headerCard);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    m_subtitleLabel = new ElaText(trId("filter.dialog.subtitle"), 12, headerCard);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);

    auto *fileRow = new QHBoxLayout();
    fileRow->setSpacing(8);
    m_filePathEdit = new ElaLineEdit(headerCard);
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setPlaceholderText(trId("filter.dialog.filePlaceholder"));
    m_browseButton = new ElaPushButton(trId("filter.dialog.browse"), headerCard);
    m_browseButton->setFixedHeight(32);
    connect(m_browseButton, &ElaPushButton::clicked, this, &FilterStudioDialog::onBrowseFile);
    fileRow->addWidget(m_filePathEdit, 1);
    fileRow->addWidget(m_browseButton);
    headerLayout->addLayout(fileRow);

    m_metaInfoLabel = new ElaText(QString(), 12, headerCard);
    m_metaInfoLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    headerLayout->addWidget(m_metaInfoLabel);
    bodyLayout->addWidget(headerCard);

    // 2. Video Player & Preview Control Bar
    m_player = new VideoPlayerWidget(body);
    m_player->setAutoSkipVisible(false);
    bodyLayout->addWidget(m_player);

    auto *previewCard = new ElaScrollPageArea(body);
    auto *previewLayout = new QHBoxLayout(previewCard);
    previewLayout->setContentsMargins(12, 8, 12, 8);
    previewLayout->setSpacing(10);

    m_genPreviewBtn = new ElaPushButton(trId("filter.preview.generate"), previewCard);
    m_genPreviewBtn->setFixedHeight(32);
    m_genPreviewBtn->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    connect(m_genPreviewBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onGeneratePreview);

    m_togglePreviewBtn = new ElaPushButton(trId("filter.preview.toggleOriginal"), previewCard);
    m_togglePreviewBtn->setFixedHeight(32);
    m_togglePreviewBtn->setEnabled(false);
    connect(m_togglePreviewBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onTogglePreviewMode);

    m_previewProgress = new ElaProgressBar(previewCard);
    m_previewProgress->setFixedHeight(6);
    m_previewProgress->setRange(0, 0);
    m_previewProgress->setVisible(false);

    m_previewStatusLabel = new ElaText(QString(), 12, previewCard);

    previewLayout->addWidget(m_genPreviewBtn);
    previewLayout->addWidget(m_togglePreviewBtn);
    previewLayout->addWidget(m_previewProgress, 1);
    previewLayout->addWidget(m_previewStatusLabel);
    previewLayout->addStretch();
    bodyLayout->addWidget(previewCard);

    // 3. Tabbed Studio Controls
    m_pivot = new ElaPivot(body);
    m_pivot->setPivotSpacing(16);
    m_stackedWidget = new QStackedWidget(body);

    // --- Tab 1: Color & 3D LUT ---
    auto *tab1Widget = new QWidget(m_stackedWidget);
    auto *tab1Layout = new QVBoxLayout(tab1Widget);
    tab1Layout->setContentsMargins(0, 8, 0, 8);
    tab1Layout->setSpacing(10);

    auto *presetCard = new ElaScrollPageArea(tab1Widget);
    auto *presetLayout = new QFormLayout(presetCard);
    presetLayout->setSpacing(8);

    m_presetCombo = new ElaComboBox(presetCard);
    m_presetCombo->addItem(trId("filter.preset.default"), static_cast<int>(ColorPreset::Default));
    m_presetCombo->addItem(trId("filter.preset.cinematic"), static_cast<int>(ColorPreset::CinematicTealOrange));
    m_presetCombo->addItem(trId("filter.preset.warmVintage"), static_cast<int>(ColorPreset::WarmVintage));
    m_presetCombo->addItem(trId("filter.preset.coolNight"), static_cast<int>(ColorPreset::CoolNight));
    m_presetCombo->addItem(trId("filter.preset.bw"), static_cast<int>(ColorPreset::BwHighContrast));
    m_presetCombo->addItem(trId("filter.preset.lowlight"), static_cast<int>(ColorPreset::LowlightBoost));
    m_presetCombo->addItem(trId("filter.preset.vivid"), static_cast<int>(ColorPreset::VividPop));
    m_presetCombo->addItem(trId("filter.preset.custom"), static_cast<int>(ColorPreset::Custom));
    connect(m_presetCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &FilterStudioDialog::onPresetChanged);
    presetLayout->addRow(new ElaText(trId("filter.preset.caption") + QStringLiteral(":"), 13, presetCard), m_presetCombo);

    auto *lutRow = new QHBoxLayout();
    m_lutPathEdit = new ElaLineEdit(presetCard);
    m_lutPathEdit->setReadOnly(true);
    m_lutPathEdit->setPlaceholderText(trId("filter.lut.placeholder"));
    m_browseLutBtn = new ElaPushButton(trId("filter.lut.browse"), presetCard);
    m_browseLutBtn->setFixedHeight(30);
    connect(m_browseLutBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onBrowseLut);
    m_clearLutBtn = new ElaPushButton(trId("filter.lut.clear"), presetCard);
    m_clearLutBtn->setFixedHeight(30);
    connect(m_clearLutBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onClearLut);
    lutRow->addWidget(m_lutPathEdit, 1);
    lutRow->addWidget(m_browseLutBtn);
    lutRow->addWidget(m_clearLutBtn);
    presetLayout->addRow(new ElaText(trId("filter.lut.caption") + QStringLiteral(":"), 13, presetCard), lutRow);
    tab1Layout->addWidget(presetCard);

    auto *colorCard = new ElaScrollPageArea(tab1Widget);
    auto *colorGrid = new QGridLayout(colorCard);
    colorGrid->setHorizontalSpacing(24);
    colorGrid->setVerticalSpacing(10);

    auto createDoubleSpin = [this, colorCard](double min, double max, double val, double step, int decimals) {
        auto *spin = new ElaDoubleSpinBox(colorCard);
        spin->setRange(min, max);
        spin->setValue(val);
        spin->setSingleStep(step);
        spin->setDecimals(decimals);
        spin->setFixedWidth(110);
        connect(spin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onVideoControlChanged);
        return spin;
    };

    m_brightnessSpin = createDoubleSpin(-1.0, 1.0, 0.0, 0.02, 2);
    m_contrastSpin = createDoubleSpin(0.0, 2.0, 1.0, 0.05, 2);
    m_saturationSpin = createDoubleSpin(0.0, 3.0, 1.0, 0.05, 2);
    m_gammaSpin = createDoubleSpin(0.2, 3.0, 1.0, 0.05, 2);
    m_temperatureSpin = createDoubleSpin(-1.0, 1.0, 0.0, 0.05, 2);
    m_tintSpin = createDoubleSpin(-1.0, 1.0, 0.0, 0.05, 2);

    colorGrid->addWidget(new ElaText(trId("filter.color.brightness") + QStringLiteral(":"), 13, colorCard), 0, 0);
    colorGrid->addWidget(m_brightnessSpin, 0, 1);
    colorGrid->addWidget(new ElaText(trId("filter.color.contrast") + QStringLiteral(":"), 13, colorCard), 0, 2);
    colorGrid->addWidget(m_contrastSpin, 0, 3);

    colorGrid->addWidget(new ElaText(trId("filter.color.saturation") + QStringLiteral(":"), 13, colorCard), 1, 0);
    colorGrid->addWidget(m_saturationSpin, 1, 1);
    colorGrid->addWidget(new ElaText(trId("filter.color.gamma") + QStringLiteral(":"), 13, colorCard), 1, 2);
    colorGrid->addWidget(m_gammaSpin, 1, 3);

    colorGrid->addWidget(new ElaText(trId("filter.color.temperature") + QStringLiteral(":"), 13, colorCard), 2, 0);
    colorGrid->addWidget(m_temperatureSpin, 2, 1);
    colorGrid->addWidget(new ElaText(trId("filter.color.tint") + QStringLiteral(":"), 13, colorCard), 2, 2);
    colorGrid->addWidget(m_tintSpin, 2, 3);
    tab1Layout->addWidget(colorCard);

    m_stackedWidget->addWidget(tab1Widget);

    // --- Tab 2: Enhancements ---
    auto *tab2Widget = new QWidget(m_stackedWidget);
    auto *tab2Layout = new QVBoxLayout(tab2Widget);
    tab2Layout->setContentsMargins(0, 8, 0, 8);
    tab2Layout->setSpacing(10);

    auto *enhCard = new ElaScrollPageArea(tab2Widget);
    auto *enhForm = new QFormLayout(enhCard);
    enhForm->setSpacing(12);

    auto *denoiseRow = new QHBoxLayout();
    m_denoiseCheck = new ElaCheckBox(trId("filter.video.denoise"), enhCard);
    m_denoiseSpin = new ElaDoubleSpinBox(enhCard);
    m_denoiseSpin->setRange(1.0, 10.0);
    m_denoiseSpin->setValue(4.0);
    m_denoiseSpin->setSingleStep(0.5);
    m_denoiseSpin->setFixedWidth(110);
    connect(m_denoiseCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onVideoControlChanged);
    connect(m_denoiseSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onVideoControlChanged);
    denoiseRow->addWidget(m_denoiseCheck);
    denoiseRow->addSpacing(16);
    denoiseRow->addWidget(new ElaText(trId("filter.video.strength") + QStringLiteral(":"), 12, enhCard));
    denoiseRow->addWidget(m_denoiseSpin);
    denoiseRow->addStretch();
    enhForm->addRow(denoiseRow);

    auto *sharpenRow = new QHBoxLayout();
    m_sharpenCheck = new ElaCheckBox(trId("filter.video.sharpen"), enhCard);
    m_sharpenSpin = new ElaDoubleSpinBox(enhCard);
    m_sharpenSpin->setRange(0.1, 2.5);
    m_sharpenSpin->setValue(0.8);
    m_sharpenSpin->setSingleStep(0.1);
    m_sharpenSpin->setFixedWidth(110);
    connect(m_sharpenCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onVideoControlChanged);
    connect(m_sharpenSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onVideoControlChanged);
    sharpenRow->addWidget(m_sharpenCheck);
    sharpenRow->addSpacing(16);
    sharpenRow->addWidget(new ElaText(trId("filter.video.amount") + QStringLiteral(":"), 12, enhCard));
    sharpenRow->addWidget(m_sharpenSpin);
    sharpenRow->addStretch();
    enhForm->addRow(sharpenRow);

    auto *vignetteRow = new QHBoxLayout();
    m_vignetteCheck = new ElaCheckBox(trId("filter.video.vignette"), enhCard);
    m_vignetteSpin = new ElaDoubleSpinBox(enhCard);
    m_vignetteSpin->setRange(0.1, 1.0);
    m_vignetteSpin->setValue(0.5);
    m_vignetteSpin->setSingleStep(0.1);
    m_vignetteSpin->setFixedWidth(110);
    connect(m_vignetteCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onVideoControlChanged);
    connect(m_vignetteSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onVideoControlChanged);
    vignetteRow->addWidget(m_vignetteCheck);
    vignetteRow->addSpacing(16);
    vignetteRow->addWidget(new ElaText(trId("filter.video.radius") + QStringLiteral(":"), 12, enhCard));
    vignetteRow->addWidget(m_vignetteSpin);
    vignetteRow->addStretch();
    enhForm->addRow(vignetteRow);

    m_deinterlaceCheck = new ElaCheckBox(trId("filter.video.deinterlace"), enhCard);
    connect(m_deinterlaceCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onVideoControlChanged);
    enhForm->addRow(m_deinterlaceCheck);

    tab2Layout->addWidget(enhCard);
    m_stackedWidget->addWidget(tab2Widget);

    // --- Tab 3: Audio FX ---
    auto *tab3Widget = new QWidget(m_stackedWidget);
    auto *tab3Layout = new QVBoxLayout(tab3Widget);
    tab3Layout->setContentsMargins(0, 8, 0, 8);
    tab3Layout->setSpacing(10);

    auto *audioCard = new ElaScrollPageArea(tab3Widget);
    auto *audioForm = new QFormLayout(audioCard);
    audioForm->setSpacing(12);

    auto *loudnormRow = new QHBoxLayout();
    m_loudnormCheck = new ElaCheckBox(trId("filter.audio.loudnorm"), audioCard);
    m_loudnormStandardCombo = new ElaComboBox(audioCard);
    m_loudnormStandardCombo->addItem(trId("filter.audio.standard.youtube"), static_cast<int>(LoudnessStandard::YouTube));
    m_loudnormStandardCombo->addItem(trId("filter.audio.standard.podcast"), static_cast<int>(LoudnessStandard::ApplePodcast));
    m_loudnormStandardCombo->addItem(trId("filter.audio.standard.ebur128"), static_cast<int>(LoudnessStandard::EbuR128));
    m_loudnormStandardCombo->addItem(trId("filter.audio.standard.atsc"), static_cast<int>(LoudnessStandard::AtscA85));
    m_loudnormStandardCombo->addItem(trId("filter.audio.standard.custom"), static_cast<int>(LoudnessStandard::Custom));
    m_targetLufsSpin = new ElaDoubleSpinBox(audioCard);
    m_targetLufsSpin->setRange(-30.0, -8.0);
    m_targetLufsSpin->setValue(-14.0);
    m_targetLufsSpin->setSuffix(QStringLiteral(" LUFS"));
    m_targetLufsSpin->setFixedWidth(110);

    connect(m_loudnormCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onAudioControlChanged);
    connect(m_loudnormStandardCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &FilterStudioDialog::onStandardChanged);
    connect(m_targetLufsSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onAudioControlChanged);

    loudnormRow->addWidget(m_loudnormCheck);
    loudnormRow->addWidget(m_loudnormStandardCombo);
    loudnormRow->addWidget(m_targetLufsSpin);
    loudnormRow->addStretch();
    audioForm->addRow(loudnormRow);

    auto *audioDenoiseRow = new QHBoxLayout();
    m_audioDenoiseCheck = new ElaCheckBox(trId("filter.audio.denoise"), audioCard);
    m_noiseReductionSpin = new ElaDoubleSpinBox(audioCard);
    m_noiseReductionSpin->setRange(6.0, 25.0);
    m_noiseReductionSpin->setValue(12.0);
    m_noiseReductionSpin->setSuffix(QStringLiteral(" dB"));
    m_noiseReductionSpin->setFixedWidth(110);
    connect(m_audioDenoiseCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onAudioControlChanged);
    connect(m_noiseReductionSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onAudioControlChanged);
    audioDenoiseRow->addWidget(m_audioDenoiseCheck);
    audioDenoiseRow->addWidget(m_noiseReductionSpin);
    audioDenoiseRow->addStretch();
    audioForm->addRow(audioDenoiseRow);

    auto *eqRow = new QHBoxLayout();
    m_highpassCheck = new ElaCheckBox(trId("filter.audio.highpass"), audioCard);
    m_highpassSpin = new ElaSpinBox(audioCard);
    m_highpassSpin->setRange(40, 300);
    m_highpassSpin->setValue(80);
    m_highpassSpin->setSuffix(QStringLiteral(" Hz"));
    m_highpassSpin->setFixedWidth(100);

    m_lowpassCheck = new ElaCheckBox(trId("filter.audio.lowpass"), audioCard);
    m_lowpassSpin = new ElaSpinBox(audioCard);
    m_lowpassSpin->setRange(4000, 20000);
    m_lowpassSpin->setValue(12000);
    m_lowpassSpin->setSuffix(QStringLiteral(" Hz"));
    m_lowpassSpin->setFixedWidth(100);

    connect(m_highpassCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onAudioControlChanged);
    connect(m_highpassSpin, &ElaSpinBox::valueChanged, this, &FilterStudioDialog::onAudioControlChanged);
    connect(m_lowpassCheck, &ElaCheckBox::toggled, this, &FilterStudioDialog::onAudioControlChanged);
    connect(m_lowpassSpin, &ElaSpinBox::valueChanged, this, &FilterStudioDialog::onAudioControlChanged);

    eqRow->addWidget(m_highpassCheck);
    eqRow->addWidget(m_highpassSpin);
    eqRow->addSpacing(16);
    eqRow->addWidget(m_lowpassCheck);
    eqRow->addWidget(m_lowpassSpin);
    eqRow->addStretch();
    audioForm->addRow(eqRow);

    auto *volumeRow = new QHBoxLayout();
    m_volumeDbSpin = new ElaDoubleSpinBox(audioCard);
    m_volumeDbSpin->setRange(-24.0, 24.0);
    m_volumeDbSpin->setValue(0.0);
    m_volumeDbSpin->setSingleStep(0.5);
    m_volumeDbSpin->setSuffix(QStringLiteral(" dB"));
    m_volumeDbSpin->setFixedWidth(110);
    connect(m_volumeDbSpin, &ElaDoubleSpinBox::valueChanged, this, &FilterStudioDialog::onAudioControlChanged);
    volumeRow->addWidget(new ElaText(trId("filter.audio.volume") + QStringLiteral(":"), 13, audioCard));
    volumeRow->addWidget(m_volumeDbSpin);
    volumeRow->addStretch();
    audioForm->addRow(volumeRow);

    tab3Layout->addWidget(audioCard);
    m_stackedWidget->addWidget(tab3Widget);

    m_tabColorTitle = trId("filter.tab.color");
    m_tabEnhancementsTitle = trId("filter.tab.enhancements");
    m_tabAudioTitle = trId("filter.tab.audio");
    m_pivot->appendPivot(m_tabColorTitle);
    m_pivot->appendPivot(m_tabEnhancementsTitle);
    m_pivot->appendPivot(m_tabAudioTitle);
    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    bodyLayout->addWidget(m_pivot);
    bodyLayout->addWidget(m_stackedWidget);

    // 4. Filter Syntax Preview Card
    auto *syntaxCard = new ElaScrollPageArea(body);
    auto *syntaxLayout = new QVBoxLayout(syntaxCard);
    syntaxLayout->setContentsMargins(12, 8, 12, 8);
    syntaxLayout->setSpacing(4);

    m_vfPreviewLabel = new ElaText(QString(), 11, syntaxCard);
    m_vfPreviewLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-family: monospace;"));
    m_afPreviewLabel = new ElaText(QString(), 11, syntaxCard);
    m_afPreviewLabel->setStyleSheet(QStringLiteral("color: #107c41; font-family: monospace;"));
    syntaxLayout->addWidget(m_vfPreviewLabel);
    syntaxLayout->addWidget(m_afPreviewLabel);
    bodyLayout->addWidget(syntaxCard);

    scroll->setWidget(body);
    rootLayout->addWidget(scroll, 1);

    // 5. Bottom Action Bar
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(10);

    m_resetBtn = new ElaPushButton(trId("filter.action.reset"), this);
    m_resetBtn->setFixedHeight(36);
    connect(m_resetBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onResetAll);

    m_applyBtn = new ElaPushButton(trId("filter.action.apply"), this);
    m_applyBtn->setFixedHeight(36);
    m_applyBtn->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0078d4; color: white;"));
    connect(m_applyBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onApplyToConvert);

    m_exportBtn = new ElaPushButton(trId("filter.action.export"), this);
    m_exportBtn->setFixedHeight(36);
    connect(m_exportBtn, &ElaPushButton::clicked, this, &FilterStudioDialog::onExportClicked);

    m_closeBtn = new ElaPushButton(trId("dialog.close"), this);
    m_closeBtn->setFixedHeight(36);
    connect(m_closeBtn, &ElaPushButton::clicked, this, &QDialog::accept);

    bottomRow->addWidget(m_resetBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_applyBtn);
    bottomRow->addWidget(m_exportBtn);
    bottomRow->addWidget(m_closeBtn);
    rootLayout->addLayout(bottomRow);

    m_previewProcess = new QProcess(this);
    connect(m_previewProcess, &QProcess::finished, this, &FilterStudioDialog::onPreviewProcessFinished);

    updateFilterStrings();
}

FilterStudioDialog::~FilterStudioDialog()
{
    if (m_previewProcess && m_previewProcess->state() != QProcess::NotRunning) {
        m_previewProcess->kill();
        m_previewProcess->waitForFinished(500);
    }
    if (!m_previewSampleFile.isEmpty() && QFile::exists(m_previewSampleFile)) {
        QFile::remove(m_previewSampleFile);
    }
}

void FilterStudioDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void FilterStudioDialog::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString file = urls.first().toLocalFile();
    if (!file.isEmpty()) {
        loadFile(file);
        event->acceptProposedAction();
    }
}

void FilterStudioDialog::onBrowseFile()
{
    const QString file = QFileDialog::getOpenFileName(this, trId("filter.dialog.browse"), QString(),
                                                      QStringLiteral("Media Files (*.mp4 *.mkv *.mov *.avi *.webm *.mp3 *.flac *.wav);;All Files (*.*)"));
    if (!file.isEmpty()) {
        loadFile(file);
    }
}

void FilterStudioDialog::loadFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    m_inputFile = filePath;
    m_filePathEdit->setText(filePath);
    m_player->loadMedia(filePath);

    if (m_probeService) {
        const MediaFile meta = m_probeService->probe(filePath);
        if (meta.isValid()) {
            const VideoStream v = meta.primaryVideo();
            const AudioStream a = meta.primaryAudio();
            m_metaInfoLabel->setText(QStringLiteral("%1x%2 | %3 fps | V: %4 | A: %5 | %6s")
                                         .arg(v.width)
                                         .arg(v.height)
                                         .arg(QString::number(v.frameRate, 'f', 1))
                                         .arg(v.codec.isEmpty() ? QStringLiteral("-") : v.codec)
                                         .arg(a.codec.isEmpty() ? QStringLiteral("-") : a.codec)
                                         .arg(QString::number(meta.duration, 'f', 1)));
        }
    }
    updateFilterStrings();
}

void FilterStudioDialog::onBrowseLut()
{
    const QString file = QFileDialog::getOpenFileName(this, trId("filter.lut.browse"), QString(),
                                                      QStringLiteral("3D LUT Files (*.cube *.3dl *.dat *.m3d);;All Files (*.*)"));
    if (!file.isEmpty()) {
        m_lutPathEdit->setText(file);
        m_videoOpts.lutFilePath = file;
        updateFilterStrings();
    }
}

void FilterStudioDialog::onClearLut()
{
    m_lutPathEdit->clear();
    m_videoOpts.lutFilePath.clear();
    updateFilterStrings();
}

void FilterStudioDialog::onPresetChanged(int index)
{
    const auto p = static_cast<ColorPreset>(m_presetCombo->itemData(index).toInt());
    m_videoOpts.applyPreset(p);
    syncUiFromOptions();
    updateFilterStrings();
}

void FilterStudioDialog::onStandardChanged(int index)
{
    const auto s = static_cast<LoudnessStandard>(m_loudnormStandardCombo->itemData(index).toInt());
    m_audioOpts.applyStandard(s);
    m_targetLufsSpin->setValue(m_audioOpts.targetLufs);
    m_loudnormCheck->setChecked(true);
    updateFilterStrings();
}

void FilterStudioDialog::onVideoControlChanged()
{
    m_videoOpts.brightness = m_brightnessSpin->value();
    m_videoOpts.contrast = m_contrastSpin->value();
    m_videoOpts.saturation = m_saturationSpin->value();
    m_videoOpts.gamma = m_gammaSpin->value();
    m_videoOpts.temperature = m_temperatureSpin->value();
    m_videoOpts.tint = m_tintSpin->value();

    m_videoOpts.enableDenoise = m_denoiseCheck->isChecked();
    m_videoOpts.denoiseStrength = m_denoiseSpin->value();
    m_videoOpts.enableSharpen = m_sharpenCheck->isChecked();
    m_videoOpts.sharpenAmount = m_sharpenSpin->value();
    m_videoOpts.enableVignette = m_vignetteCheck->isChecked();
    m_videoOpts.vignetteStrength = m_vignetteSpin->value();
    m_videoOpts.enableDeinterlace = m_deinterlaceCheck->isChecked();

    updateFilterStrings();
}

void FilterStudioDialog::onAudioControlChanged()
{
    m_audioOpts.enableLoudnorm = m_loudnormCheck->isChecked();
    m_audioOpts.targetLufs = m_targetLufsSpin->value();

    m_audioOpts.enableDenoise = m_audioDenoiseCheck->isChecked();
    m_audioOpts.noiseReductionDb = m_noiseReductionSpin->value();

    m_audioOpts.enableHighpass = m_highpassCheck->isChecked();
    m_audioOpts.highpassFreq = m_highpassSpin->value();

    m_audioOpts.enableLowpass = m_lowpassCheck->isChecked();
    m_audioOpts.lowpassFreq = m_lowpassSpin->value();

    m_audioOpts.volumeDb = m_volumeDbSpin->value();

    updateFilterStrings();
}

void FilterStudioDialog::syncUiFromOptions()
{
    m_brightnessSpin->blockSignals(true);
    m_contrastSpin->blockSignals(true);
    m_saturationSpin->blockSignals(true);
    m_gammaSpin->blockSignals(true);
    m_temperatureSpin->blockSignals(true);
    m_tintSpin->blockSignals(true);
    m_denoiseCheck->blockSignals(true);
    m_denoiseSpin->blockSignals(true);
    m_sharpenCheck->blockSignals(true);
    m_sharpenSpin->blockSignals(true);
    m_vignetteCheck->blockSignals(true);
    m_vignetteSpin->blockSignals(true);
    m_deinterlaceCheck->blockSignals(true);

    m_brightnessSpin->setValue(m_videoOpts.brightness);
    m_contrastSpin->setValue(m_videoOpts.contrast);
    m_saturationSpin->setValue(m_videoOpts.saturation);
    m_gammaSpin->setValue(m_videoOpts.gamma);
    m_temperatureSpin->setValue(m_videoOpts.temperature);
    m_tintSpin->setValue(m_videoOpts.tint);

    m_denoiseCheck->setChecked(m_videoOpts.enableDenoise);
    m_denoiseSpin->setValue(m_videoOpts.denoiseStrength);
    m_sharpenCheck->setChecked(m_videoOpts.enableSharpen);
    m_sharpenSpin->setValue(m_videoOpts.sharpenAmount);
    m_vignetteCheck->setChecked(m_videoOpts.enableVignette);
    m_vignetteSpin->setValue(m_videoOpts.vignetteStrength);
    m_deinterlaceCheck->setChecked(m_videoOpts.enableDeinterlace);

    m_brightnessSpin->blockSignals(false);
    m_contrastSpin->blockSignals(false);
    m_saturationSpin->blockSignals(false);
    m_gammaSpin->blockSignals(false);
    m_temperatureSpin->blockSignals(false);
    m_tintSpin->blockSignals(false);
    m_denoiseCheck->blockSignals(false);
    m_denoiseSpin->blockSignals(false);
    m_sharpenCheck->blockSignals(false);
    m_sharpenSpin->blockSignals(false);
    m_vignetteCheck->blockSignals(false);
    m_vignetteSpin->blockSignals(false);
    m_deinterlaceCheck->blockSignals(false);
}

void FilterStudioDialog::onResetAll()
{
    m_videoOpts.reset();
    m_audioOpts.reset();
    m_lutPathEdit->clear();
    m_presetCombo->setCurrentIndex(0);
    m_loudnormStandardCombo->setCurrentIndex(0);

    m_loudnormCheck->setChecked(false);
    m_audioDenoiseCheck->setChecked(false);
    m_highpassCheck->setChecked(false);
    m_lowpassCheck->setChecked(false);
    m_volumeDbSpin->setValue(0.0);

    syncUiFromOptions();
    updateFilterStrings();
}

void FilterStudioDialog::updateFilterStrings()
{
    const QString vf = MediaFilterStudio::compileVideoFilters(m_videoOpts);
    const QString af = MediaFilterStudio::compileAudioFilters(m_audioOpts);

    m_vfPreviewLabel->setText(vf.isEmpty() ? QStringLiteral("-vf: (None)")
                                          : QStringLiteral("-vf %1").arg(vf));
    m_afPreviewLabel->setText(af.isEmpty() ? QStringLiteral("-af: (None)")
                                          : QStringLiteral("-af %1").arg(af));
}

void FilterStudioDialog::onGeneratePreview()
{
    if (m_inputFile.isEmpty()) {
        QMessageBox::warning(this, trId("filter.dialog.title"), trId("filter.error.noFile"));
        return;
    }

    if (m_previewProcess->state() != QProcess::NotRunning) {
        m_previewProcess->kill();
        m_previewProcess->waitForFinished(500);
    }

    const QString tempDir = QDir::tempPath();
    m_previewSampleFile = QStringLiteral("%1/ffmedia_filter_preview.mp4").arg(tempDir);
    if (QFile::exists(m_previewSampleFile)) {
        QFile::remove(m_previewSampleFile);
    }

    m_previewProgress->setVisible(true);
    m_previewStatusLabel->setText(trId("filter.preview.generating"));
    m_genPreviewBtn->setEnabled(false);

    const double curPos = m_player->currentPositionSec();
    const QString ffmpeg = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QStringList args = MediaFilterStudio::buildSamplePreviewCommand(
        m_inputFile, m_previewSampleFile, m_videoOpts, m_audioOpts, curPos, 5.0);

    m_previewProcess->start(ffmpeg, args);
}

void FilterStudioDialog::onPreviewProcessFinished(int exitCode)
{
    m_previewProgress->setVisible(false);
    m_genPreviewBtn->setEnabled(true);

    if (exitCode == 0 && QFile::exists(m_previewSampleFile)) {
        m_previewStatusLabel->setText(trId("filter.preview.finished"));
        m_togglePreviewBtn->setEnabled(true);
        m_isPreviewMode = true;
        m_togglePreviewBtn->setText(trId("filter.preview.toggleOriginal"));
        m_player->loadMedia(m_previewSampleFile);
        m_player->play();
    } else {
        m_previewStatusLabel->setText(trId("filter.preview.failed"));
    }
}

void FilterStudioDialog::onTogglePreviewMode()
{
    if (m_previewSampleFile.isEmpty() || !QFile::exists(m_previewSampleFile)) return;

    m_isPreviewMode = !m_isPreviewMode;
    if (m_isPreviewMode) {
        m_togglePreviewBtn->setText(trId("filter.preview.toggleOriginal"));
        m_player->loadMedia(m_previewSampleFile);
        m_player->play();
    } else {
        m_togglePreviewBtn->setText(trId("filter.preview.toggleGraded"));
        m_player->loadMedia(m_inputFile);
        m_player->play();
    }
}

void FilterStudioDialog::onApplyToConvert()
{
    const QString vf = MediaFilterStudio::compileVideoFilters(m_videoOpts);
    const QString af = MediaFilterStudio::compileAudioFilters(m_audioOpts);
    emit filtersApplied(vf, af);
    QMessageBox::information(this, trId("filter.dialog.title"), trId("filter.success.applied"));
    accept();
}

void FilterStudioDialog::onExportClicked()
{
    if (m_inputFile.isEmpty()) {
        QMessageBox::warning(this, trId("filter.dialog.title"), trId("filter.error.noFile"));
        return;
    }

    const QString ext = QFileInfo(m_inputFile).suffix().isEmpty() ? QStringLiteral("mp4") : QFileInfo(m_inputFile).suffix();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_filtered.{format}"),
                                               ext);

    const QString ffmpeg = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QStringList args = MediaFilterStudio::buildFilterExportCommand(
        m_inputFile, output, m_videoOpts, m_audioOpts, QStringLiteral("libx264"), QStringLiteral("aac"), 18);

    auto *job = new Job();
    job->setName(QStringLiteral("Render Filter Studio: %1").arg(QFileInfo(m_inputFile).fileName()));
    job->setCommand(ffmpeg, args);
    m_queue->addJob(job);

    QMessageBox::information(this, trId("filter.dialog.title"), trId("filter.success.enqueued"));
    accept();
}

void FilterStudioDialog::retranslateUi()
{
    setWindowTitle(trId("filter.dialog.title"));
    m_titleLabel->setText(trId("filter.dialog.title"));
    m_subtitleLabel->setText(trId("filter.dialog.subtitle"));
    m_browseButton->setText(trId("filter.dialog.browse"));
    m_genPreviewBtn->setText(trId("filter.preview.generate"));
    m_togglePreviewBtn->setText(m_isPreviewMode ? trId("filter.preview.toggleOriginal")
                                               : trId("filter.preview.toggleGraded"));

    const int currentIdx = m_pivot->getCurrentIndex();
    if (!m_tabColorTitle.isEmpty()) {
        m_pivot->removePivot(m_tabColorTitle);
        m_pivot->removePivot(m_tabEnhancementsTitle);
        m_pivot->removePivot(m_tabAudioTitle);
    }
    m_tabColorTitle = trId("filter.tab.color");
    m_tabEnhancementsTitle = trId("filter.tab.enhancements");
    m_tabAudioTitle = trId("filter.tab.audio");
    m_pivot->appendPivot(m_tabColorTitle);
    m_pivot->appendPivot(m_tabEnhancementsTitle);
    m_pivot->appendPivot(m_tabAudioTitle);
    m_pivot->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);

    m_browseLutBtn->setText(trId("filter.lut.browse"));
    m_clearLutBtn->setText(trId("filter.lut.clear"));

    m_denoiseCheck->setText(trId("filter.video.denoise"));
    m_sharpenCheck->setText(trId("filter.video.sharpen"));
    m_vignetteCheck->setText(trId("filter.video.vignette"));
    m_deinterlaceCheck->setText(trId("filter.video.deinterlace"));

    m_loudnormCheck->setText(trId("filter.audio.loudnorm"));
    m_audioDenoiseCheck->setText(trId("filter.audio.denoise"));
    m_highpassCheck->setText(trId("filter.audio.highpass"));
    m_lowpassCheck->setText(trId("filter.audio.lowpass"));

    m_resetBtn->setText(trId("filter.action.reset"));
    m_applyBtn->setText(trId("filter.action.apply"));
    m_exportBtn->setText(trId("filter.action.export"));
    m_closeBtn->setText(trId("dialog.close"));
    updateFilterStrings();
}
