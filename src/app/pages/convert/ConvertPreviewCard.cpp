#include "ConvertPreviewCard.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaText.h"
#include "i18n/TrId.h"

ConvertPreviewCard::ConvertPreviewCard(QWidget *parent)
    : ElaScrollPageArea(parent)
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *previewLayout = new QVBoxLayout(this);
    previewLayout->setContentsMargins(18, 16, 18, 16);
    previewLayout->setSpacing(10);

    auto *previewTopRow = new QHBoxLayout();
    m_previewCardTitle = new ElaText(QString(), 15, this);
    m_previewCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_estLabel = new ElaText(QString(), 13, this);
    m_estLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    m_estLabel->setVisible(false);
    previewTopRow->addWidget(m_previewCardTitle);
    previewTopRow->addStretch();
    previewTopRow->addWidget(m_estLabel);
    previewLayout->addLayout(previewTopRow);

    m_previewCaption = new ElaText(QString(), 13, this);
    m_previewEdit = new ElaLineEdit(this);
    m_previewEdit->setReadOnly(true);
    previewLayout->addWidget(m_previewCaption);
    previewLayout->addWidget(m_previewEdit);

    retranslateUi();
}

void ConvertPreviewCard::setCommand(const QString &command)
{
    m_previewEdit->setText(command);
}

void ConvertPreviewCard::setEstimate(const QString &text, bool visible)
{
    m_estLabel->setText(text);
    m_estLabel->setVisible(visible);
}

void ConvertPreviewCard::retranslateUi()
{
    if (m_previewCardTitle) m_previewCardTitle->setText(trId("convert.card.preview"));
    m_previewCaption->setText(trId("convert.preview") + QStringLiteral(":"));
}
