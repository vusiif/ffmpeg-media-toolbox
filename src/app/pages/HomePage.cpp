#include "HomePage.h"

#include "i18n/TrId.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaPushButton.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegLocator.h"

HomePage::HomePage(FFmpegLocator *locator, FFmpegCapabilities *caps, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_caps(caps)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(20);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(32);
    m_subtitleLabel = new ElaText(QString(), 13, this);
    m_subtitleLabel->setTextPixelSize(15);
    root->addWidget(m_titleLabel);
    root->addWidget(m_subtitleLabel);

    auto *statusCard = new QFrame(this);
    statusCard->setFrameShape(QFrame::StyledPanel);
    auto *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(16, 16, 16, 16);
    statusLayout->setSpacing(10);

    m_statusTitle = new ElaText(QString(), 13, statusCard);
    m_statusTitle->setTextPixelSize(15);

    auto *versionRow = new QHBoxLayout();
    m_versionCaption = new ElaText(QString(), 13, statusCard);
    m_versionValue = new ElaText(QString(), 13, statusCard);
    versionRow->addWidget(m_versionCaption);
    versionRow->addWidget(m_versionValue);
    versionRow->addStretch();

    m_rescanButton = new ElaPushButton(statusCard);
    connect(m_rescanButton, &ElaPushButton::clicked, this, [=]() {
        m_locator->autoDetect();
        m_caps->scan();
        m_versionValue->setText(m_locator->ffmpegVersion());
    });

    statusLayout->addWidget(m_statusTitle);
    statusLayout->addLayout(versionRow);
    statusLayout->addWidget(m_rescanButton, 0, Qt::AlignRight);
    root->addWidget(statusCard);

    root->addStretch();

    retranslateUi();
}

void HomePage::retranslateUi()
{
    m_titleLabel->setText(trId("home.title"));
    m_subtitleLabel->setText(trId("home.dropHint"));
    m_statusTitle->setText(trId("ffmpeg.status"));
    m_versionCaption->setText(trId("ffmpeg.version") + QStringLiteral(":"));
    m_versionValue->setText(m_locator->ffmpegVersion());
    m_rescanButton->setText(trId("ffmpeg.rescan"));
}
