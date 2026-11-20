#pragma once

#include <QString>

#include "ElaScrollPageArea.h"

class ElaText;
class ElaLineEdit;

class ConvertPreviewCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit ConvertPreviewCard(QWidget *parent = nullptr);
    ~ConvertPreviewCard() override = default;

    void setCommand(const QString &command);
    void setEstimate(const QString &text, bool visible);
    void retranslateUi();

private:
    ElaText *m_previewCardTitle = nullptr;
    ElaText *m_estLabel = nullptr;
    ElaText *m_previewCaption = nullptr;
    ElaLineEdit *m_previewEdit = nullptr;
};
