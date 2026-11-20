#pragma once

#include <QList>
#include <QString>

#include "ElaScrollPageArea.h"

class ElaCheckBox;

class BatchMatrixCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    struct TargetConfig {
        QString name;
        QString format;
        QString vcodec;
        QString acodec;
        int crf;
        QString scale;
        QString suffix;
        bool isAudio;
    };

    explicit BatchMatrixCard(QWidget *parent = nullptr);
    ~BatchMatrixCard() override = default;

    bool isMatrixEnabled() const;
    QList<TargetConfig> activeTargets() const;
    void retranslateUi();

private:
    ElaCheckBox *m_matrixToggle = nullptr;
    QWidget *m_matrixContainer = nullptr;
    ElaCheckBox *m_target1Check = nullptr;
    ElaCheckBox *m_target2Check = nullptr;
    ElaCheckBox *m_target3Check = nullptr;
    ElaCheckBox *m_target4Check = nullptr;
};
