#pragma once

#include "ElaDialog.h"

#include <QFutureWatcher>
#include <QLabel>
#include <QString>

class FFmpegLocator;
class ElaText;
class ElaPushButton;
class ElaDoubleSpinBox;
class QScrollArea;

class ABCompareDialog : public ElaDialog
{
    Q_OBJECT

public:
    explicit ABCompareDialog(const QString &sourcePath,
                             const QString &previewPath,
                             double duration,
                             const FFmpegLocator *locator,
                             QWidget *parent = nullptr);
    ~ABCompareDialog() override;

    void retranslateUi();

private slots:
    void captureFrames();
    void onCaptureFinished();

private:
    void setupUi();
    static bool extractFrame(const QString &ffmpegPath, const QString &input,
                             double timeSec, const QString &outputImage);

    QString m_sourcePath;
    QString m_previewPath;
    double m_duration = 0.0;
    const FFmpegLocator *m_locator = nullptr;

    QString m_sourceTempImg;
    QString m_previewTempImg;

    ElaText *m_titleText = nullptr;
    ElaText *m_timeCaption = nullptr;
    ElaDoubleSpinBox *m_timeSpin = nullptr;
    ElaPushButton *m_pct10Btn = nullptr;
    ElaPushButton *m_pct25Btn = nullptr;
    ElaPushButton *m_pct50Btn = nullptr;
    ElaPushButton *m_pct75Btn = nullptr;
    ElaPushButton *m_refreshBtn = nullptr;
    ElaPushButton *m_closeBtn = nullptr;

    ElaText *m_leftHeader = nullptr;
    ElaText *m_rightHeader = nullptr;
    QLabel *m_leftImageLabel = nullptr;
    QLabel *m_rightImageLabel = nullptr;
    QScrollArea *m_leftScroll = nullptr;
    QScrollArea *m_rightScroll = nullptr;
    ElaText *m_statusLabel = nullptr;

    QFutureWatcher<QPair<bool, bool>> *m_watcher = nullptr;
};
