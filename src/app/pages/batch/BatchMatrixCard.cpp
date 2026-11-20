#include "BatchMatrixCard.h"

#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "i18n/TrId.h"

BatchMatrixCard::BatchMatrixCard(QWidget *parent)
    : ElaScrollPageArea(parent)
{
    auto *cardLayout = new QVBoxLayout(this);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(10);

    m_matrixToggle = new ElaCheckBox(trId("batch.matrix.title"), this);
    cardLayout->addWidget(m_matrixToggle);

    m_matrixContainer = new QWidget(this);
    auto *matrixLayout = new QVBoxLayout(m_matrixContainer);
    matrixLayout->setContentsMargins(0, 8, 0, 0);
    matrixLayout->setSpacing(6);

    m_target1Check = new ElaCheckBox(QStringLiteral("Web MP4 (1080p H.264/AAC, CRF 23)"), m_matrixContainer);
    m_target1Check->setChecked(true);
    m_target2Check = new ElaCheckBox(QStringLiteral("Mobile MP4 (720p H.264/AAC, CRF 24)"), m_matrixContainer);
    m_target3Check = new ElaCheckBox(QStringLiteral("High Efficiency (HEVC/H.265, CRF 26)"), m_matrixContainer);
    m_target4Check = new ElaCheckBox(QStringLiteral("Audio Extraction (MP3 320kbps)"), m_matrixContainer);

    matrixLayout->addWidget(m_target1Check);
    matrixLayout->addWidget(m_target2Check);
    matrixLayout->addWidget(m_target3Check);
    matrixLayout->addWidget(m_target4Check);

    m_matrixContainer->setVisible(false);
    cardLayout->addWidget(m_matrixContainer);

    connect(m_matrixToggle, &ElaCheckBox::toggled, m_matrixContainer, &QWidget::setVisible);
}

bool BatchMatrixCard::isMatrixEnabled() const
{
    return m_matrixToggle->isChecked();
}

QList<BatchMatrixCard::TargetConfig> BatchMatrixCard::activeTargets() const
{
    QList<TargetConfig> targets;
    if (m_target1Check->isChecked()) {
        targets.append({QStringLiteral("1080p MP4"), QStringLiteral("mp4"), QStringLiteral("libx264"),
                        QStringLiteral("aac"), 23, QStringLiteral("1920:-2"), QStringLiteral("_1080p"), false});
    }
    if (m_target2Check->isChecked()) {
        targets.append({QStringLiteral("720p MP4"), QStringLiteral("mp4"), QStringLiteral("libx264"),
                        QStringLiteral("aac"), 24, QStringLiteral("1280:-2"), QStringLiteral("_720p"), false});
    }
    if (m_target3Check->isChecked()) {
        targets.append({QStringLiteral("HEVC"), QStringLiteral("mp4"), QStringLiteral("libx265"),
                        QStringLiteral("aac"), 26, QString(), QStringLiteral("_hevc"), false});
    }
    if (m_target4Check->isChecked()) {
        targets.append({QStringLiteral("MP3 Audio"), QStringLiteral("mp3"), QString(),
                        QStringLiteral("libmp3lame"), 0, QString(), QStringLiteral("_audio"), true});
    }
    return targets;
}

void BatchMatrixCard::retranslateUi()
{
    m_matrixToggle->setText(trId("batch.matrix.title"));
}
