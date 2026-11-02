#include "VideoPage.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QFutureWatcher>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPivot.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollBar.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"
#include "app/widgets/VideoPlayerWidget.h"
#include "app/dialogs/FilterStudioDialog.h"
#include "app/dialogs/SubtitleStudioDialog.h"
#include "app/dialogs/PipCompositorDialog.h"
#include "app/dialogs/LosslessCutDialog.h"
#include "media/MediaProbeService.h"

VideoPage::VideoPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, MediaProbeService *probeService, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
    , m_probeService(probeService)
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(14);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    // Common Source Video Input Card
    auto *inputCard = new ElaScrollPageArea(this);
    inputCard->setMinimumHeight(0);
    inputCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *inputCardLayout = new QHBoxLayout(inputCard);
    inputCardLayout->setContentsMargins(18, 12, 18, 12);
    inputCardLayout->setSpacing(12);

    m_inputCaption = new ElaText(QString(), 13, inputCard);
    m_inputEdit = new ElaLineEdit(inputCard);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(inputCard);
    connect(m_browseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(trId("nav.video"),
                                        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        if (!path.isEmpty()) {
            m_inputFile = path;
            m_inputEdit->setText(path);
            if (m_trimPlayer) {
                m_trimPlayer->loadMedia(path);
            }
            detectKeyframes();
        }
    });
    inputCardLayout->addWidget(m_inputCaption);
    inputCardLayout->addWidget(m_inputEdit, 1);
    inputCardLayout->addWidget(m_browseButton);
    root->addWidget(inputCard);

    // Segmented Navigation Switcher
    m_pivot = new ElaPivot(this);
    m_pivot->setPivotSpacing(16);
    m_pivot->setMarkWidth(90);
    root->addWidget(m_pivot);

    m_stackedWidget = new QStackedWidget(this);
    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    root->addWidget(m_stackedWidget, 1);

    auto createTab = [this](QWidget *&bodyWidget, QVBoxLayout *&bodyLayout) {
        auto *tab = new QWidget(m_stackedWidget);
        auto *tabLayout = new QVBoxLayout(tab);
        tabLayout->setContentsMargins(0, 4, 0, 0);
        tabLayout->setSpacing(0);

        auto *scroll = new ElaScrollArea(tab);
        scroll->setWidgetResizable(true);
        scroll->setIsGrabGesture(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        bodyWidget = new QWidget(scroll);
        bodyWidget->setStyleSheet(QStringLiteral("background-color: transparent;"));
        bodyLayout = new QVBoxLayout(bodyWidget);
        bodyLayout->setContentsMargins(0, 0, 10, 0);
        bodyLayout->setSpacing(14);

        scroll->setWidget(bodyWidget);
        tabLayout->addWidget(scroll);
        return tab;
    };

    auto createCard = [](QWidget *parent, ElaText *&titleLabel) {
        auto *card = new ElaScrollPageArea(parent);
        card->setMinimumHeight(0);
        card->setMaximumHeight(QWIDGETSIZE_MAX);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(10);

        titleLabel = new ElaText(QString(), 15, card);
        titleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        layout->addWidget(titleLabel);
        return card;
    };

    auto createCardButton = [](ElaScrollPageArea *card) {
        auto *button = new ElaPushButton(card);
        button->setFixedHeight(36);
        button->setMinimumWidth(140);
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        btnRow->addWidget(button);
        if (auto *layout = qobject_cast<QVBoxLayout *>(card->layout())) {
            layout->addLayout(btnRow);
        }
        return button;
    };

    auto makeCaption = [](ElaScrollPageArea *card) {
        return new ElaText(QString(), 13, card);
    };

    // =========================================================================
    // Tab 1: 剪辑与调整 (Clip & Adjust)
    // =========================================================================
    QWidget *bodyClip = nullptr;
    QVBoxLayout *bodyClipLayout = nullptr;
    QWidget *tabClip = createTab(bodyClip, bodyClipLayout);

    // Trim Card
    m_trimCard = createCard(bodyClip, m_trimTitleLabel);
    auto *trimCardLayout = qobject_cast<QVBoxLayout *>(m_trimCard->layout());

    m_trimPlayer = new VideoPlayerWidget(m_trimCard);
    m_trimPlayer->setAutoSkipVisible(false);
    trimCardLayout->addWidget(m_trimPlayer);

    auto *kfRow = new QHBoxLayout();
    kfRow->setSpacing(8);
    m_prevKeyframeBtn = new ElaPushButton(QStringLiteral("⏮"), m_trimCard);
    m_prevKeyframeBtn->setFixedSize(36, 30);
    m_prevKeyframeBtn->setEnabled(false);
    m_nextKeyframeBtn = new ElaPushButton(QStringLiteral("⏭"), m_trimCard);
    m_nextKeyframeBtn->setFixedSize(36, 30);
    m_nextKeyframeBtn->setEnabled(false);
    m_snapInButton = new ElaPushButton(m_trimCard);
    m_snapInButton->setFixedHeight(30);
    m_snapInButton->setEnabled(false);
    m_snapOutButton = new ElaPushButton(m_trimCard);
    m_snapOutButton->setFixedHeight(30);
    m_snapOutButton->setEnabled(false);
    m_keyframeStatusLabel = new ElaText(QString(), 12, m_trimCard);
    m_openStudioButton = new ElaPushButton(m_trimCard);
    m_openStudioButton->setFixedHeight(30);
    m_openStudioButton->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));

    kfRow->addWidget(m_prevKeyframeBtn);
    kfRow->addWidget(m_nextKeyframeBtn);
    kfRow->addWidget(m_snapInButton);
    kfRow->addWidget(m_snapOutButton);
    kfRow->addWidget(m_keyframeStatusLabel, 1);
    kfRow->addWidget(m_openStudioButton);
    trimCardLayout->addLayout(kfRow);

    auto *markerRow = new QHBoxLayout();
    markerRow->setSpacing(8);
    m_setInButton = new ElaPushButton(m_trimCard);
    m_setInButton->setFixedHeight(32);
    m_setOutButton = new ElaPushButton(m_trimCard);
    m_setOutButton->setFixedHeight(32);
    m_previewTrimButton = new ElaPushButton(m_trimCard);
    m_previewTrimButton->setFixedHeight(32);
    markerRow->addWidget(m_setInButton);
    markerRow->addWidget(m_setOutButton);
    markerRow->addWidget(m_previewTrimButton);
    markerRow->addStretch();
    trimCardLayout->addLayout(markerRow);

    auto *trimForm = new QFormLayout();
    trimForm->setSpacing(8);
    m_trimStartCaption = makeCaption(m_trimCard);
    m_trimStart = new ElaDoubleSpinBox(m_trimCard);
    m_trimStart->setRange(0, 86400);
    m_trimStart->setDecimals(1);
    m_trimEndCaption = makeCaption(m_trimCard);
    m_trimEnd = new ElaDoubleSpinBox(m_trimCard);
    m_trimEnd->setRange(0, 86400);
    m_trimEnd->setDecimals(1);
    m_trimFormatCaption = makeCaption(m_trimCard);
    m_trimFormat = new ElaComboBox(m_trimCard);
    m_trimFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("mov"), QStringLiteral("webm")});
    m_trimModeCaption = makeCaption(m_trimCard);
    m_trimMode = new ElaComboBox(m_trimCard);
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::KeyframeLossless));
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::SmartCut));
    m_trimMode->addItem(QString(), static_cast<int>(CutMode::AccurateReencode));
    m_trimCopyCaption = makeCaption(m_trimCard);
    m_trimCopy = new ElaCheckBox(m_trimCard);
    trimForm->addRow(m_trimStartCaption, m_trimStart);
    trimForm->addRow(m_trimEndCaption, m_trimEnd);
    trimForm->addRow(m_trimModeCaption, m_trimMode);
    trimForm->addRow(m_trimFormatCaption, m_trimFormat);
    trimForm->addRow(m_trimCopyCaption, m_trimCopy);
    trimCardLayout->addLayout(trimForm);

    connect(m_setInButton, &ElaPushButton::clicked, this, [this]() {
        const double pos = m_trimPlayer->currentPositionSec();
        m_trimStart->setValue(pos);
        if (m_trimEnd->value() <= pos && m_trimPlayer->durationSec() > pos) {
            m_trimEnd->setValue(m_trimPlayer->durationSec());
        }
        m_trimPlayer->setMarkRange(m_trimStart->value(), m_trimEnd->value());
    });

    connect(m_setOutButton, &ElaPushButton::clicked, this, [this]() {
        const double pos = m_trimPlayer->currentPositionSec();
        m_trimEnd->setValue(pos);
        m_trimPlayer->setMarkRange(m_trimStart->value(), m_trimEnd->value());
    });

    connect(m_previewTrimButton, &ElaPushButton::clicked, this, [this]() {
        m_trimPlayer->seekToSec(m_trimStart->value());
        m_trimPlayer->play();
    });

    connect(m_trimStart, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_trimPlayer->setMarkRange(val, m_trimEnd->value());
        m_trimPlayer->seekToSec(val);
    });
    connect(m_trimEnd, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_trimPlayer->setMarkRange(m_trimStart->value(), val);
        updateKeyframeStatus();
    });
    connect(m_trimStart, &ElaDoubleSpinBox::valueChanged, this, [this](double) {
        updateKeyframeStatus();
    });
    connect(m_prevKeyframeBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double cur = m_trimPlayer->currentPositionSec();
        const double target = SmartCutEngine::findPrevKeyframe(cur - 0.05, m_keyframes);
        m_trimPlayer->seekToSec(target);
    });
    connect(m_nextKeyframeBtn, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double cur = m_trimPlayer->currentPositionSec();
        const double target = SmartCutEngine::findNextKeyframe(cur + 0.05, m_keyframes);
        m_trimPlayer->seekToSec(target);
    });
    connect(m_snapInButton, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double snapped = SmartCutEngine::findNearestKeyframe(m_trimStart->value(), m_keyframes);
        m_trimStart->setValue(snapped);
        m_trimPlayer->seekToSec(snapped);
    });
    connect(m_snapOutButton, &ElaPushButton::clicked, this, [this]() {
        if (m_keyframes.isEmpty()) return;
        const double snapped = SmartCutEngine::findNearestKeyframe(m_trimEnd->value(), m_keyframes);
        m_trimEnd->setValue(snapped);
    });
    connect(m_openStudioButton, &ElaPushButton::clicked, this, &VideoPage::openLosslessCutStudio);

    m_trimButton = createCardButton(m_trimCard);
    connect(m_trimButton, &ElaPushButton::clicked, this, &VideoPage::trim);
    bodyClipLayout->addWidget(m_trimCard);

    // Adjust Card (crop / resize / rotate)
    m_adjCard = createCard(bodyClip, m_adjTitleLabel);
    auto *adjForm = new QFormLayout();
    adjForm->setSpacing(8);
    m_adjCropCaption = makeCaption(m_adjCard);
    auto *cropRow = new QHBoxLayout();
    m_adjCropX = new ElaSpinBox(m_adjCard);
    m_adjCropY = new ElaSpinBox(m_adjCard);
    m_adjCropW = new ElaSpinBox(m_adjCard);
    m_adjCropH = new ElaSpinBox(m_adjCard);
    for (ElaSpinBox *spin : {m_adjCropX, m_adjCropY, m_adjCropW, m_adjCropH}) {
        spin->setRange(0, 10000);
        cropRow->addWidget(spin);
    }
    adjForm->addRow(m_adjCropCaption, cropRow);

    auto *cropActionRow = new QHBoxLayout();
    m_autoCropButton = new ElaPushButton(m_adjCard);
    m_autoCropStatus = new ElaText(QString(), 12, m_adjCard);
    m_autoCropStatus->setStyleSheet(QStringLiteral("color: #0078d4;"));
    cropActionRow->addWidget(m_autoCropButton);
    cropActionRow->addWidget(m_autoCropStatus);
    cropActionRow->addStretch();
    adjForm->addRow(new ElaText(QString(), 13, m_adjCard), cropActionRow);
    connect(m_autoCropButton, &ElaPushButton::clicked, this, &VideoPage::detectAutoCrop);
    m_adjResizeCaption = makeCaption(m_adjCard);
    auto *resizeRow = new QHBoxLayout();
    m_adjResizeW = new ElaSpinBox(m_adjCard);
    m_adjResizeH = new ElaSpinBox(m_adjCard);
    for (ElaSpinBox *spin : {m_adjResizeW, m_adjResizeH}) {
        spin->setRange(0, 10000);
        resizeRow->addWidget(spin);
    }
    adjForm->addRow(m_adjResizeCaption, resizeRow);
    m_adjRotateCaption = makeCaption(m_adjCard);
    m_adjRotateCombo = new ElaComboBox(m_adjCard);
    m_adjRotateCombo->addItems({QStringLiteral("0"), QStringLiteral("90"),
                                QStringLiteral("180"), QStringLiteral("270")});
    adjForm->addRow(m_adjRotateCaption, m_adjRotateCombo);
    m_adjQualityCaption = makeCaption(m_adjCard);
    m_adjQualitySpin = new ElaSpinBox(m_adjCard);
    m_adjQualitySpin->setRange(0, 51);
    m_adjQualitySpin->setValue(23);
    adjForm->addRow(m_adjQualityCaption, m_adjQualitySpin);
    qobject_cast<QVBoxLayout *>(m_adjCard->layout())->addLayout(adjForm);
    m_adjButton = createCardButton(m_adjCard);
    connect(m_adjButton, &ElaPushButton::clicked, this, &VideoPage::adjust);
    bodyClipLayout->addWidget(m_adjCard);

    // Join Card
    m_joinCard = createCard(bodyClip, m_joinTitleLabel);
    auto *joinCardLayout = qobject_cast<QVBoxLayout *>(m_joinCard->layout());
    m_joinList = new QListWidget(m_joinCard);
    m_joinList->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background-color: transparent;"
        "  border: 1px solid rgba(128, 128, 128, 0.25);"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 8px;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(0, 120, 212, 0.25);"
        "}"
    ));
    m_joinList->setMinimumHeight(100);
    joinCardLayout->addWidget(m_joinList);
    auto *joinButtons = new QHBoxLayout();
    m_joinAddButton = new ElaPushButton(m_joinCard);
    m_joinRemoveButton = new ElaPushButton(m_joinCard);
    connect(m_joinAddButton, &ElaPushButton::clicked, this, [=]() {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, trId("video.join"), QString(),
            QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi);;All files (*)"));
        for (const QString &path : paths) {
            m_joinFiles.append(path);
            m_joinList->addItem(QFileInfo(path).fileName());
        }
    });
    connect(m_joinRemoveButton, &ElaPushButton::clicked, this, [=]() {
        const QList<QListWidgetItem *> selected = m_joinList->selectedItems();
        for (QListWidgetItem *item : selected) {
            const int row = m_joinList->row(item);
            m_joinFiles.removeAt(row);
            delete m_joinList->takeItem(row);
        }
    });
    joinButtons->addWidget(m_joinAddButton);
    joinButtons->addWidget(m_joinRemoveButton);
    joinButtons->addStretch();
    joinCardLayout->addLayout(joinButtons);
    m_joinButton = createCardButton(m_joinCard);
    connect(m_joinButton, &ElaPushButton::clicked, this, &VideoPage::joinVideos);
    bodyClipLayout->addWidget(m_joinCard);

    bodyClipLayout->addStretch();
    m_stackedWidget->addWidget(tabClip);

    // =========================================================================
    // Tab 2: 音频处理 (Audio Tools)
    // =========================================================================
    QWidget *bodyAudio = nullptr;
    QVBoxLayout *bodyAudioLayout = nullptr;
    QWidget *tabAudio = createTab(bodyAudio, bodyAudioLayout);

    // Extract Audio Card
    m_audioCard = createCard(bodyAudio, m_audioTitleLabel);
    auto *audioForm = new QFormLayout();
    audioForm->setSpacing(8);
    m_audioFormatCaption = makeCaption(m_audioCard);
    m_audioFormat = new ElaComboBox(m_audioCard);
    m_audioFormat->addItems({QStringLiteral("mp3"), QStringLiteral("aac"), QStringLiteral("flac"),
                             QStringLiteral("wav"), QStringLiteral("opus"), QStringLiteral("m4a")});
    m_audioBitrateCaption = makeCaption(m_audioCard);
    m_audioBitrate = new ElaSpinBox(m_audioCard);
    m_audioBitrate->setRange(0, 512);
    m_audioBitrate->setValue(192);
    audioForm->addRow(m_audioFormatCaption, m_audioFormat);
    audioForm->addRow(m_audioBitrateCaption, m_audioBitrate);
    qobject_cast<QVBoxLayout *>(m_audioCard->layout())->addLayout(audioForm);
    m_audioButton = createCardButton(m_audioCard);
    connect(m_audioButton, &ElaPushButton::clicked, this, &VideoPage::extractAudio);
    bodyAudioLayout->addWidget(m_audioCard);

    // Mute Card
    m_muteCard = createCard(bodyAudio, m_muteTitleLabel);
    auto *muteForm = new QFormLayout();
    muteForm->setSpacing(8);
    m_muteFormatCaption = makeCaption(m_muteCard);
    m_muteFormat = new ElaComboBox(m_muteCard);
    m_muteFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("webm")});
    muteForm->addRow(m_muteFormatCaption, m_muteFormat);
    qobject_cast<QVBoxLayout *>(m_muteCard->layout())->addLayout(muteForm);
    m_muteButton = createCardButton(m_muteCard);
    connect(m_muteButton, &ElaPushButton::clicked, this, &VideoPage::mute);
    bodyAudioLayout->addWidget(m_muteCard);

    bodyAudioLayout->addStretch();
    m_stackedWidget->addWidget(tabAudio);

    // =========================================================================
    // Tab 3: 水印与字幕 (Overlay & Subtitles)
    // =========================================================================
    QWidget *bodyEffects = nullptr;
    QVBoxLayout *bodyEffectsLayout = nullptr;
    QWidget *tabEffects = createTab(bodyEffects, bodyEffectsLayout);

    // Filter Studio Card
    m_filterStudioCard = createCard(bodyEffects, m_filterStudioTitleLabel);
    auto *filterStudioLayout = qobject_cast<QVBoxLayout *>(m_filterStudioCard->layout());
    auto *filterStudioRow = new QHBoxLayout();
    auto *filterStudioDesc = new ElaText(trId("filter.dialog.subtitle"), 13, m_filterStudioCard);
    filterStudioDesc->setStyleSheet(QStringLiteral("color: #888888;"));
    m_filterStudioButton = new ElaPushButton(trId("filter.action.openStudio"), m_filterStudioCard);
    m_filterStudioButton->setFixedHeight(34);
    m_filterStudioButton->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0078d4; color: white;"));
    connect(m_filterStudioButton, &ElaPushButton::clicked, this, &VideoPage::openFilterStudioDialog);
    filterStudioRow->addWidget(filterStudioDesc, 1);
    filterStudioRow->addWidget(m_filterStudioButton);
    filterStudioLayout->addLayout(filterStudioRow);
    bodyEffectsLayout->addWidget(m_filterStudioCard);

    // Watermark Card
    m_wmCard = createCard(bodyEffects, m_wmTitleLabel);
    auto *wmForm = new QFormLayout();
    wmForm->setSpacing(8);
    auto *wmPathRow = new QHBoxLayout();
    m_wmPath = new ElaLineEdit(m_wmCard);
    m_wmPath->setReadOnly(true);
    m_wmBrowseButton = new ElaPushButton(m_wmCard);
    connect(m_wmBrowseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(trId("video.watermark"),
                                        QStringLiteral("Image files (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)"));
        if (!path.isEmpty()) {
            m_wmPath->setText(path);
        }
    });
    wmPathRow->addWidget(m_wmPath, 1);
    wmPathRow->addWidget(m_wmBrowseButton);
    m_wmImageCaption = makeCaption(m_wmCard);
    m_wmPositionCaption = makeCaption(m_wmCard);
    m_wmPosition = new ElaComboBox(m_wmCard);
    m_wmPosition->addItem(QString(), QStringLiteral("bottom-right"));
    m_wmPosition->addItem(QString(), QStringLiteral("bottom-left"));
    m_wmPosition->addItem(QString(), QStringLiteral("top-right"));
    m_wmPosition->addItem(QString(), QStringLiteral("top-left"));
    m_wmPosition->addItem(QString(), QStringLiteral("center"));
    m_wmOpacityCaption = makeCaption(m_wmCard);
    m_wmOpacity = new ElaSpinBox(m_wmCard);
    m_wmOpacity->setRange(0, 100);
    m_wmOpacity->setValue(100);
    wmForm->addRow(m_wmImageCaption, wmPathRow);
    wmForm->addRow(m_wmPositionCaption, m_wmPosition);
    wmForm->addRow(m_wmOpacityCaption, m_wmOpacity);
    qobject_cast<QVBoxLayout *>(m_wmCard->layout())->addLayout(wmForm);
    m_wmButton = createCardButton(m_wmCard);
    connect(m_wmButton, &ElaPushButton::clicked, this, &VideoPage::watermark);
    bodyEffectsLayout->addWidget(m_wmCard);

    // Picture-in-Picture Card
    m_pipCard = createCard(bodyEffects, m_pipTitleLabel);
    auto *pipForm = new QFormLayout();
    pipForm->setSpacing(8);
    auto *pipPathRow = new QHBoxLayout();
    m_pipPath = new ElaLineEdit(m_pipCard);
    m_pipPath->setReadOnly(true);
    m_pipBrowseButton = new ElaPushButton(m_pipCard);
    connect(m_pipBrowseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(
            trId("video.pip"),
            QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        if (!path.isEmpty()) {
            m_pipFile = path;
            m_pipPath->setText(path);
        }
    });
    pipPathRow->addWidget(m_pipPath, 1);
    pipPathRow->addWidget(m_pipBrowseButton);
    m_pipVideoCaption = makeCaption(m_pipCard);
    pipForm->addRow(m_pipVideoCaption, pipPathRow);
    m_pipSizeCaption = makeCaption(m_pipCard);
    auto *pipSizeRow = new QHBoxLayout();
    m_pipWidth = new ElaSpinBox(m_pipCard);
    m_pipWidth->setRange(0, 10000);
    m_pipWidth->setValue(320);
    m_pipHeight = new ElaSpinBox(m_pipCard);
    m_pipHeight->setRange(0, 10000);
    m_pipHeight->setValue(0);
    pipSizeRow->addWidget(m_pipWidth);
    pipSizeRow->addWidget(m_pipHeight);
    pipForm->addRow(m_pipSizeCaption, pipSizeRow);
    m_pipPositionCaption = makeCaption(m_pipCard);
    m_pipPosition = new ElaComboBox(m_pipCard);
    m_pipPosition->addItem(QString(), QStringLiteral("bottom-right"));
    m_pipPosition->addItem(QString(), QStringLiteral("bottom-left"));
    m_pipPosition->addItem(QString(), QStringLiteral("top-right"));
    m_pipPosition->addItem(QString(), QStringLiteral("top-left"));
    m_pipPosition->addItem(QString(), QStringLiteral("center"));
    pipForm->addRow(m_pipPositionCaption, m_pipPosition);
    m_pipMarginCaption = makeCaption(m_pipCard);
    m_pipMargin = new ElaSpinBox(m_pipCard);
    m_pipMargin->setRange(0, 500);
    m_pipMargin->setValue(10);
    pipForm->addRow(m_pipMarginCaption, m_pipMargin);
    m_pipQualityCaption = makeCaption(m_pipCard);
    m_pipQualitySpin = new ElaSpinBox(m_pipCard);
    m_pipQualitySpin->setRange(0, 51);
    m_pipQualitySpin->setValue(23);
    pipForm->addRow(m_pipQualityCaption, m_pipQualitySpin);
    qobject_cast<QVBoxLayout *>(m_pipCard->layout())->addLayout(pipForm);
    m_openPipStudioBtn = new ElaPushButton(trId("video.pip.openStudio"), m_pipCard);
    m_openPipStudioBtn->setFixedHeight(36);
    connect(m_openPipStudioBtn, &ElaPushButton::clicked, this, &VideoPage::openPipCompositorStudio);

    m_pipButton = createCardButton(m_pipCard);
    connect(m_pipButton, &ElaPushButton::clicked, this, &VideoPage::pip);

    auto *pipBtnRow = new QHBoxLayout();
    pipBtnRow->addWidget(m_openPipStudioBtn);
    pipBtnRow->addWidget(m_pipButton);
    qobject_cast<QVBoxLayout *>(m_pipCard->layout())->addLayout(pipBtnRow);

    bodyEffectsLayout->addWidget(m_pipCard);

    // Subtitle Card
    m_subCard = createCard(bodyEffects, m_subTitleLabel);
    auto *subForm = new QFormLayout();
    subForm->setSpacing(8);
    auto *subPathRow = new QHBoxLayout();
    m_subPath = new ElaLineEdit(m_subCard);
    m_subPath->setReadOnly(true);
    m_subPathButton = new ElaPushButton(m_subCard);
    connect(m_subPathButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(
            trId("video.subtitle"),
            QStringLiteral("Subtitle files (*.srt *.ass *.ssa *.vtt);;All files (*)"));
        if (!path.isEmpty()) {
            m_subtitleFile = path;
            m_subPath->setText(path);
        }
    });
    subPathRow->addWidget(m_subPath, 1);
    subPathRow->addWidget(m_subPathButton);
    m_subCaption = makeCaption(m_subCard);
    subForm->addRow(m_subCaption, subPathRow);
    m_subBurnCheck = new ElaCheckBox(m_subCard);
    subForm->addRow(new ElaText(QString(), 13, m_subCard), m_subBurnCheck);
    m_subQualityCaption = makeCaption(m_subCard);
    m_subQualitySpin = new ElaSpinBox(m_subCard);
    m_subQualitySpin->setRange(0, 51);
    m_subQualitySpin->setValue(23);
    m_subQualityCaption->setVisible(false);
    m_subQualitySpin->setVisible(false);
    subForm->addRow(m_subQualityCaption, m_subQualitySpin);
    connect(m_subBurnCheck, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_subQualityCaption->setVisible(checked);
        m_subQualitySpin->setVisible(checked);
    });
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subForm);
    m_openSubStudioBtn = new ElaPushButton(trId("video.sub.openStudio"), m_subCard);
    m_openSubStudioBtn->setFixedHeight(36);
    connect(m_openSubStudioBtn, &ElaPushButton::clicked, this, &VideoPage::openSubtitleStudio);

    m_subButton = createCardButton(m_subCard);
    connect(m_subButton, &ElaPushButton::clicked, this, &VideoPage::subtitleMux);

    auto *subBtnRow = new QHBoxLayout();
    subBtnRow->addWidget(m_openSubStudioBtn);
    subBtnRow->addWidget(m_subButton);
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subBtnRow);

    bodyEffectsLayout->addWidget(m_subCard);

    bodyEffectsLayout->addStretch();
    m_stackedWidget->addWidget(tabEffects);

    // =========================================================================
    // Tab 4: 动图与帧提取 (Frames & Animation)
    // =========================================================================
    QWidget *bodyFrames = nullptr;
    QVBoxLayout *bodyFramesLayout = nullptr;
    QWidget *tabFrames = createTab(bodyFrames, bodyFramesLayout);

    // Screenshot Card
    m_shotCard = createCard(bodyFrames, m_shotTitleLabel);
    auto *shotForm = new QFormLayout();
    shotForm->setSpacing(8);
    m_shotTimeCaption = makeCaption(m_shotCard);
    m_shotTime = new ElaDoubleSpinBox(m_shotCard);
    m_shotTime->setRange(0, 86400);
    m_shotTime->setDecimals(1);
    m_shotFormatCaption = makeCaption(m_shotCard);
    m_shotFormat = new ElaComboBox(m_shotCard);
    m_shotFormat->addItems({QStringLiteral("png"), QStringLiteral("jpeg"), QStringLiteral("webp")});
    shotForm->addRow(m_shotTimeCaption, m_shotTime);
    shotForm->addRow(m_shotFormatCaption, m_shotFormat);
    qobject_cast<QVBoxLayout *>(m_shotCard->layout())->addLayout(shotForm);
    m_shotButton = createCardButton(m_shotCard);
    connect(m_shotButton, &ElaPushButton::clicked, this, &VideoPage::screenshot);
    bodyFramesLayout->addWidget(m_shotCard);

    // GIF Card
    m_gifCard = createCard(bodyFrames, m_gifTitleLabel);
    auto *gifForm = new QFormLayout();
    gifForm->setSpacing(8);
    m_gifStartCaption = makeCaption(m_gifCard);
    m_gifStart = new ElaDoubleSpinBox(m_gifCard);
    m_gifStart->setRange(0, 86400);
    m_gifStart->setDecimals(1);
    m_gifEndCaption = makeCaption(m_gifCard);
    m_gifEnd = new ElaDoubleSpinBox(m_gifCard);
    m_gifEnd->setRange(0, 86400);
    m_gifEnd->setDecimals(1);
    m_gifFpsCaption = makeCaption(m_gifCard);
    m_gifFps = new ElaSpinBox(m_gifCard);
    m_gifFps->setRange(1, 60);
    m_gifFps->setValue(12);
    m_gifWidthCaption = makeCaption(m_gifCard);
    m_gifWidth = new ElaSpinBox(m_gifCard);
    m_gifWidth->setRange(0, 4096);
    m_gifWidth->setValue(480);
    gifForm->addRow(m_gifStartCaption, m_gifStart);
    gifForm->addRow(m_gifEndCaption, m_gifEnd);
    gifForm->addRow(m_gifFpsCaption, m_gifFps);
    gifForm->addRow(m_gifWidthCaption, m_gifWidth);
    qobject_cast<QVBoxLayout *>(m_gifCard->layout())->addLayout(gifForm);
    m_gifButton = createCardButton(m_gifCard);
    connect(m_gifButton, &ElaPushButton::clicked, this, &VideoPage::makeGif);
    bodyFramesLayout->addWidget(m_gifCard);

    // Image Sequence Card
    m_seqCard = createCard(bodyFrames, m_seqTitleLabel);
    auto *seqForm = new QFormLayout();
    seqForm->setSpacing(8);
    m_seqModeCaption = makeCaption(m_seqCard);
    m_seqMode = new ElaComboBox(m_seqCard);
    m_seqMode->addItems({QString(), QString()});
    m_seqPatternCaption = makeCaption(m_seqCard);
    m_seqPattern = new ElaLineEdit(m_seqCard);
    m_seqPattern->setText(QStringLiteral("frame_%04d.png"));
    m_seqFpsCaption = makeCaption(m_seqCard);
    m_seqFps = new ElaDoubleSpinBox(m_seqCard);
    m_seqFps->setRange(0.1, 240);
    m_seqFps->setValue(25);
    seqForm->addRow(m_seqModeCaption, m_seqMode);
    seqForm->addRow(m_seqPatternCaption, m_seqPattern);
    seqForm->addRow(m_seqFpsCaption, m_seqFps);
    qobject_cast<QVBoxLayout *>(m_seqCard->layout())->addLayout(seqForm);
    m_seqButton = createCardButton(m_seqCard);
    connect(m_seqButton, &ElaPushButton::clicked, this, &VideoPage::sequence);
    bodyFramesLayout->addWidget(m_seqCard);

    bodyFramesLayout->addStretch();
    m_stackedWidget->addWidget(tabFrames);

    retranslateUi();
}

VideoPage::~VideoPage()
{
    if (m_probeProcess && m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }
}

void VideoPage::openLosslessCutStudio()
{
    auto *dlg = new LosslessCutDialog(m_locator, m_probeService, m_queue, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        dlg->loadFile(m_inputFile);
    }
    dlg->exec();
}

void VideoPage::openFilterStudioDialog()
{
    auto *dlg = new FilterStudioDialog(m_locator, m_probeService, m_queue, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        dlg->loadFile(m_inputFile);
    }
    dlg->exec();
}

void VideoPage::detectKeyframes()
{
    if (m_inputFile.isEmpty()) return;
    if (m_probeProcess && m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }
    if (!m_probeProcess) {
        m_probeProcess = new QProcess(this);
        connect(m_probeProcess, &QProcess::finished, this, &VideoPage::onKeyframeProbeFinished);
    }
    const QString ffprobe = m_locator ? m_locator->ffprobePath() : QStringLiteral("ffprobe");
    const QStringList args = SmartCutEngine::buildKeyframeProbeCommand(ffprobe, m_inputFile);
    m_probeProcess->start(args.first(), args.mid(1));
}

void VideoPage::onKeyframeProbeFinished(int exitCode)
{
    if (exitCode != 0 || !m_probeProcess) return;
    const QString output = QString::fromUtf8(m_probeProcess->readAllStandardOutput());
    m_keyframes = SmartCutEngine::parseKeyframeOutput(output);
    if (m_trimPlayer) {
        m_trimPlayer->setKeyframes(m_keyframes);
    }
    const bool hasKeyframes = !m_keyframes.isEmpty();
    if (m_prevKeyframeBtn) m_prevKeyframeBtn->setEnabled(hasKeyframes);
    if (m_nextKeyframeBtn) m_nextKeyframeBtn->setEnabled(hasKeyframes);
    if (m_snapInButton) m_snapInButton->setEnabled(hasKeyframes);
    if (m_snapOutButton) m_snapOutButton->setEnabled(hasKeyframes);
    updateKeyframeStatus();
}

void VideoPage::updateKeyframeStatus()
{
    if (!m_keyframeStatusLabel) return;
    if (m_keyframes.isEmpty()) {
        m_keyframeStatusLabel->clear();
        return;
    }
    const double inVal = m_trimStart ? m_trimStart->value() : 0.0;
    const double outVal = m_trimEnd ? m_trimEnd->value() : 0.0;
    const bool inAligned = SmartCutEngine::isKeyframeAligned(inVal, m_keyframes);
    const bool outAligned = SmartCutEngine::isKeyframeAligned(outVal, m_keyframes);

    if (inAligned && outAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.bothAligned").arg(m_keyframes.size()));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #107c41; font-weight: bold;"));
    } else if (inAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.inAlignedOnly").arg(QString::number(SmartCutEngine::findNearestKeyframe(outVal, m_keyframes), 'f', 2)));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));
    } else if (outAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.outAlignedOnly").arg(QString::number(SmartCutEngine::findNearestKeyframe(inVal, m_keyframes), 'f', 2)));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));
    } else {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.neitherAligned").arg(m_keyframes.size()));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #d83b01;"));
    }
}

QString VideoPage::browseFile(const QString &title, const QString &filters)
{
    return QFileDialog::getOpenFileName(this, title, QString(), filters);
}

void VideoPage::enqueue(const QString &name, const QStringList &args)
{
    if (args.isEmpty()) return;
    auto *job = new Job();
    job->setName(name);
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void VideoPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VideoPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    m_inputFile = urls.first().toLocalFile();
    m_inputEdit->setText(m_inputFile);
    if (m_trimPlayer && !m_inputFile.isEmpty()) {
        m_trimPlayer->loadMedia(m_inputFile);
    }
    detectKeyframes();
    event->acceptProposedAction();
}

void VideoPage::trim()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_trimFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_trimFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_trim.{format}"), ext);
    const auto mode = static_cast<CutMode>(m_trimMode->currentData().toInt());

    QStringList args;
    if (mode == CutMode::KeyframeLossless || m_trimCopy->isChecked()) {
        args = SmartCutEngine::buildLosslessCut(m_inputFile, output,
                                                m_trimStart->value(), m_trimEnd->value(),
                                                m_trimFormat->currentText());
    } else if (mode == CutMode::SmartCut) {
        SmartCutPlan plan = SmartCutEngine::planSmartCut(m_inputFile, output,
                                                         m_trimStart->value(), m_trimEnd->value(),
                                                         m_keyframes);
        if (plan.isValid && !plan.needsConcat) {
            args = SmartCutEngine::buildChunkCommand(m_inputFile, plan.chunks.first());
        } else {
            args = SmartCutEngine::buildLosslessCut(m_inputFile, output,
                                                    m_trimStart->value(), m_trimEnd->value(),
                                                    m_trimFormat->currentText());
        }
    } else {
        args = SmartCutEngine::buildAccurateReencode(m_inputFile, output,
                                                    m_trimStart->value(), m_trimEnd->value(),
                                                    QString(), 18, m_trimFormat->currentText());
    }

    enqueue(trId("video.job.trim").arg(QFileInfo(m_inputFile).fileName()), args);
}

void VideoPage::extractAudio()
{
    if (m_inputFile.isEmpty()) return;
    const QString format = m_audioFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}.{format}"), format);
    enqueue(trId("video.job.audio").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildExtractAudio(m_inputFile, output, format, QString(),
                                         m_audioBitrate->value()));
}

void VideoPage::mute()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_muteFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_muteFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_muted.{format}"), ext);
    enqueue(trId("video.job.mute").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildMute(m_inputFile, output, m_muteFormat->currentText(),
                                 QString(), -1, true));
}

void VideoPage::screenshot()
{
    if (m_inputFile.isEmpty()) return;
    const QString format = m_shotFormat->currentText();
    QString ext = format;
    if (format == QLatin1String("jpeg")) ext = QStringLiteral("jpg");
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_shot.{format}"), ext);
    enqueue(trId("video.job.screenshot").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildScreenshot(m_inputFile, output, m_shotTime->value(), format));
}

void VideoPage::makeGif()
{
    if (m_inputFile.isEmpty()) return;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}.{format}"),
                                               QStringLiteral("gif"));
    enqueue(trId("video.job.gif").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildGif(m_inputFile, output,
                                m_gifFps->value(), m_gifWidth->value(),
                                m_gifStart->value(), m_gifEnd->value()));
}

void VideoPage::joinVideos()
{
    if (m_joinFiles.size() < 2) return;
    const QString output = OutputNaming::build(m_joinFiles.first(),
                                               QStringLiteral("{name}_joined.{format}"),
                                               QStringLiteral("mp4"));
    enqueue(trId("video.job.join").arg(m_joinFiles.size()),
            m_builder->buildVideoJoin(m_joinFiles, output));
}

void VideoPage::sequence()
{
    const bool imagesToVideo = m_seqMode->currentIndex() == 0;
    const QString pattern = m_seqPattern->text().trimmed();
    if (pattern.isEmpty()) return;

    if (imagesToVideo) {
        const QString output = QFileInfo(pattern).absolutePath() + QStringLiteral("/sequence.mp4");
        enqueue(trId("video.job.seqToVideo"),
                m_builder->buildImageSequenceToVideo(pattern, output,
                                                     static_cast<int>(m_seqFps->value())));
    } else {
        if (m_inputFile.isEmpty()) return;
        const QString output = QFileInfo(m_inputFile).absolutePath() + QStringLiteral("/") + pattern;
        enqueue(trId("video.job.videoToSeq"),
                m_builder->buildVideoToImageSequence(m_inputFile, output, m_seqFps->value()));
    }
}

void VideoPage::watermark()
{
    const QString wmPath = m_wmPath->text();
    if (m_inputFile.isEmpty() || wmPath.isEmpty()) return;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_wm.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    enqueue(trId("video.job.watermark").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildWatermark(m_inputFile, wmPath, output,
                                      m_wmPosition->currentData().toString(),
                                      m_wmOpacity->value()));
}

void VideoPage::openSubtitleStudio()
{
    auto *dlg = new SubtitleStudioDialog(m_queue, m_locator, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        dlg->setVideoFile(m_inputFile);
    }
    dlg->show();
}

void VideoPage::openPipCompositorStudio()
{
    auto *dlg = new PipCompositorDialog(m_queue, m_locator, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        dlg->setMainVideo(m_inputFile);
    }
    if (!m_pipFile.isEmpty()) {
        dlg->setPipVideo(m_pipFile);
    }
    dlg->show();
}

void VideoPage::subtitleMux()
{
    if (m_inputFile.isEmpty() || m_subtitleFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_sub.{format}"),
                                               suffix);

    if (m_subBurnCheck->isChecked()) {
        enqueue(trId("video.job.subtitleBurn").arg(QFileInfo(m_subtitleFile).fileName()),
                m_builder->buildSubtitleBurnin(m_inputFile, m_subtitleFile, output,
                                               m_subQualitySpin->value()));
        return;
    }

    const auto codec = RemuxPolicy::subtitleCodecForContainer(suffix);
    if (!codec) {
        QMessageBox::warning(this, trId("video.subtitle"),
                             trId("video.subUnsupported"));
        return;
    }

    enqueue(trId("video.job.subtitle").arg(QFileInfo(m_subtitleFile).fileName()),
            m_builder->buildSubtitleMux(m_inputFile, m_subtitleFile, output,
                                        *codec));
}

void VideoPage::adjust()
{
    if (m_inputFile.isEmpty()) return;

    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_adj.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    // Empty args (nothing selected) are dropped by enqueue.
    enqueue(trId("video.job.adjust").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildVideoAdjust(
                m_inputFile, output,
                m_adjCropX->value(), m_adjCropY->value(),
                m_adjCropW->value(), m_adjCropH->value(),
                m_adjResizeW->value(), m_adjResizeH->value(),
                m_adjRotateCombo->currentText().toInt(),
                m_adjQualitySpin->value()));
}

void VideoPage::detectAutoCrop()
{
    if (m_inputFile.isEmpty()) {
        m_autoCropStatus->setText(trId("video.autoCrop.noInput"));
        return;
    }

    m_autoCropButton->setEnabled(false);
    m_autoCropStatus->setText(trId("video.autoCrop.detecting"));

    const QString inputPath = m_inputFile;
    const QString ffmpegPath = m_locator->ffmpegPath();
    const double duration = (m_trimPlayer && m_trimPlayer->durationSec() > 0)
                                ? m_trimPlayer->durationSec()
                                : 0.0;
    const double seekSec = (duration > 0.0)
                               ? qMin(1.0, duration * 0.1)
                               : 1.0;
    const QStringList args = m_builder->buildCropdetect(inputPath, seekSec, 25);

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        const QString output = watcher->result();
        watcher->deleteLater();
        m_autoCropButton->setEnabled(true);

        const FFmpegCommandBuilder::CropArea crop = FFmpegCommandBuilder::parseCropdetectOutput(output);
        if (crop.isValid()) {
            m_adjCropX->setValue(crop.x);
            m_adjCropY->setValue(crop.y);
            m_adjCropW->setValue(crop.w);
            m_adjCropH->setValue(crop.h);
            m_autoCropStatus->setText(QStringLiteral("%1: %2x%3 (+%4,+%5)")
                                          .arg(trId("video.autoCrop.found"))
                                          .arg(crop.w)
                                          .arg(crop.h)
                                          .arg(crop.x)
                                          .arg(crop.y));
        } else {
            m_autoCropStatus->setText(trId("video.autoCrop.none"));
        }
    });

    QFuture<QString> future = QtConcurrent::run([ffmpegPath, args]() -> QString {
        QProcess proc;
        proc.start(ffmpegPath, args);
        if (!proc.waitForStarted(3000)) {
            return {};
        }
        if (!proc.waitForFinished(10000)) {
            proc.kill();
            proc.waitForFinished(1000);
        }
        return QString::fromUtf8(proc.readAllStandardError());
    });
    watcher->setFuture(future);
}

void VideoPage::pip()
{
    if (m_inputFile.isEmpty() || m_pipFile.isEmpty()) return;

    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_pip.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    enqueue(trId("video.job.pip").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildPip(
                m_inputFile, m_pipFile, output,
                m_pipWidth->value(), m_pipHeight->value(),
                m_pipPosition->currentData().toString(),
                m_pipMargin->value(),
                m_pipQualitySpin->value()));
}

void VideoPage::retranslateUi()
{
    const int currentIdx = m_pivot->getCurrentIndex();
    if (!m_clipTabTitle.isEmpty()) {
        m_pivot->removePivot(m_clipTabTitle);
        m_pivot->removePivot(m_audioTabTitle);
        m_pivot->removePivot(m_effectsTabTitle);
        m_pivot->removePivot(m_framesTabTitle);
    }
    m_clipTabTitle = trId("video.tab.clip");
    m_audioTabTitle = trId("video.tab.audio");
    m_effectsTabTitle = trId("video.tab.effects");
    m_framesTabTitle = trId("video.tab.frames");
    m_pivot->appendPivot(m_clipTabTitle);
    m_pivot->appendPivot(m_audioTabTitle);
    m_pivot->appendPivot(m_effectsTabTitle);
    m_pivot->appendPivot(m_framesTabTitle);
    m_pivot->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);

    m_titleLabel->setText(trId("video.title"));
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));

    m_trimTitleLabel->setText(trId("video.trim"));
    m_trimPlayer->retranslateUi();
    m_setInButton->setText(trId("video.trim.setIn"));
    m_setOutButton->setText(trId("video.trim.setOut"));
    m_previewTrimButton->setText(trId("video.trim.preview"));
    m_trimStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_trimEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_trimModeCaption->setText(trId("cut.mode.caption") + QStringLiteral(":"));
    m_trimMode->setItemText(0, trId("cut.mode.lossless"));
    m_trimMode->setItemText(1, trId("cut.mode.smart"));
    m_trimMode->setItemText(2, trId("cut.mode.accurate"));
    m_snapInButton->setText(trId("cut.keyframe.snapIn"));
    m_snapOutButton->setText(trId("cut.keyframe.snapOut"));
    m_openStudioButton->setText(trId("cut.action.openStudio"));
    m_trimFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_trimCopyCaption->setText(trId("video.trim.copy") + QStringLiteral(":"));
    m_trimButton->setText(trId("convert.addToQueue"));
    updateKeyframeStatus();

    m_audioTitleLabel->setText(trId("video.audio"));
    m_audioFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_audioBitrateCaption->setText(trId("video.audio.bitrate") + QStringLiteral(":"));
    m_audioButton->setText(trId("convert.addToQueue"));

    m_muteTitleLabel->setText(trId("video.mute"));
    m_muteFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_muteButton->setText(trId("convert.addToQueue"));

    m_shotTitleLabel->setText(trId("video.screenshot"));
    m_shotTimeCaption->setText(trId("video.shot.time") + QStringLiteral(":"));
    m_shotFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_shotButton->setText(trId("convert.addToQueue"));

    m_gifTitleLabel->setText(trId("video.gif"));
    m_gifStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_gifEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_gifFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_gifWidthCaption->setText(trId("common.width") + QStringLiteral(":"));
    m_gifButton->setText(trId("convert.addToQueue"));

    m_joinTitleLabel->setText(trId("video.join"));
    m_joinAddButton->setText(trId("common.open"));
    m_joinRemoveButton->setText(trId("common.delete"));
    m_joinButton->setText(trId("convert.addToQueue"));

    m_seqTitleLabel->setText(trId("video.sequence"));
    m_seqModeCaption->setText(trId("common.mode") + QStringLiteral(":"));
    m_seqMode->setItemText(0, trId("video.seq.imagesToVideo"));
    m_seqMode->setItemText(1, trId("video.seq.videoToImages"));
    m_seqPatternCaption->setText(trId("video.seq.pattern") + QStringLiteral(":"));
    m_seqFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_seqButton->setText(trId("convert.addToQueue"));

    m_filterStudioTitleLabel->setText(trId("filter.dialog.title"));
    m_filterStudioButton->setText(trId("filter.action.openStudio"));
    m_wmTitleLabel->setText(trId("video.watermark"));
    m_wmImageCaption->setText(trId("common.image") + QStringLiteral(":"));
    m_wmPositionCaption->setText(trId("common.position") + QStringLiteral(":"));
    m_wmPosition->setItemText(0, trId("video.wm.bottomRight"));
    m_wmPosition->setItemText(1, trId("video.wm.bottomLeft"));
    m_wmPosition->setItemText(2, trId("video.wm.topRight"));
    m_wmPosition->setItemText(3, trId("video.wm.topLeft"));
    m_wmPosition->setItemText(4, trId("video.wm.center"));
    m_wmOpacityCaption->setText(trId("common.opacity") + QStringLiteral(":"));
    m_wmButton->setText(trId("convert.addToQueue"));
    m_wmBrowseButton->setText(trId("common.browse"));

    m_subTitleLabel->setText(trId("video.subtitle"));
    m_subCaption->setText(trId("video.sub.file") + QStringLiteral(":"));
    m_subPathButton->setText(trId("common.browse"));
    m_subBurnCheck->setText(trId("video.sub.burn"));
    m_openSubStudioBtn->setText(trId("video.sub.openStudio"));
    m_subQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_subButton->setText(trId("convert.addToQueue"));

    m_adjTitleLabel->setText(trId("video.adjust"));
    m_adjCropCaption->setText(trId("image.crop") + QStringLiteral(":"));
    m_autoCropButton->setText(trId("video.autoCrop"));
    m_adjResizeCaption->setText(trId("image.resize") + QStringLiteral(":"));
    m_adjRotateCaption->setText(trId("image.rotate") + QStringLiteral(":"));
    m_adjQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_adjButton->setText(trId("convert.addToQueue"));

    m_pipTitleLabel->setText(trId("video.pip"));
    m_pipVideoCaption->setText(trId("video.pip.video") + QStringLiteral(":"));
    m_pipBrowseButton->setText(trId("common.browse"));
    m_pipSizeCaption->setText(trId("common.size") + QStringLiteral(" (W x H):"));
    m_pipPositionCaption->setText(trId("common.position") + QStringLiteral(":"));
    m_pipPosition->setItemText(0, trId("video.wm.bottomRight"));
    m_pipPosition->setItemText(1, trId("video.wm.bottomLeft"));
    m_pipPosition->setItemText(2, trId("video.wm.topRight"));
    m_pipPosition->setItemText(3, trId("video.wm.topLeft"));
    m_pipPosition->setItemText(4, trId("video.wm.center"));
    m_pipMarginCaption->setText(trId("video.pip.margin") + QStringLiteral(":"));
    m_pipQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_openPipStudioBtn->setText(trId("video.pip.openStudio"));
    m_pipButton->setText(trId("convert.addToQueue"));
}
