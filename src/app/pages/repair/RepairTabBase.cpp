#include "RepairTabBase.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

RepairTabBase::RepairTabBase(FFmpegLocator *ffmpegLocator,
                             MediaProbeService *probeService,
                             JobQueue *jobQueue,
                             QWidget *parent)
    : QWidget(parent)
    , m_ffmpegLocator(ffmpegLocator)
    , m_probeService(probeService)
    , m_jobQueue(jobQueue)
{
}

void RepairTabBase::openFolderAndSelectFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
#ifdef Q_OS_WIN
    const QString winPath = QDir::toNativeSeparators(filePath);
    QProcess::startDetached(QStringLiteral("explorer.exe"), {QStringLiteral("/select,"), winPath});
#else
    const QString dir = QFileInfo(filePath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
#endif
}

void RepairTabBase::playMediaFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

ElaScrollPageArea *RepairTabBase::createCard(QWidget *parentWidget)
{
    auto *card = new ElaScrollPageArea(parentWidget ? parentWidget : this);
    card->setMinimumHeight(0);
    card->setMaximumHeight(QWIDGETSIZE_MAX);
    return card;
}

ElaText *RepairTabBase::makeCaption(const QString &text, QWidget *parentWidget)
{
    return new ElaText(text, 13, parentWidget ? parentWidget : this);
}

ElaPushButton *RepairTabBase::createCardButton(const QString &text, int height, QWidget *parentWidget)
{
    auto *btn = new ElaPushButton(text, parentWidget ? parentWidget : this);
    btn->setFixedHeight(height);
    return btn;
}
