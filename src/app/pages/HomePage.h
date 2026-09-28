#pragma once

#include <QWidget>

class FFmpegLocator;
class FFmpegCapabilities;
class ElaText;
class ElaPushButton;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(FFmpegLocator *locator, FFmpegCapabilities *caps, QWidget *parent = nullptr);

    void retranslateUi();

private:
    FFmpegLocator *m_locator;
    FFmpegCapabilities *m_caps;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;
    ElaText *m_statusTitle = nullptr;
    ElaText *m_versionCaption = nullptr;
    ElaText *m_versionValue = nullptr;
    ElaPushButton *m_rescanButton = nullptr;
};
