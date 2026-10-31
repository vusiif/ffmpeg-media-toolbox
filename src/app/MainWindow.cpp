#include "MainWindow.h"

#include "i18n/TrId.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QSettings>
#include <QSystemTrayIcon>

#include "ElaIcon.h"
#include "app/AppSettings.h"
#include "pages/BatchPage.h"
#include "pages/ConvertPage.h"
#include "pages/HomePage.h"
#include "pages/ImagePage.h"
#include "pages/JoinPage.h"
#include "pages/QueuePage.h"
#include "pages/RepairPage.h"
#include "pages/SettingsPage.h"
#include "pages/VideoPage.h"
#include "pages/AudioPage.h"
#include "repair/UntruncLocator.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "app/RecentFiles.h"
#include "i18n/LanguageManager.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"
#include <QRegularExpression>

namespace {
QString formatShortVersion(const QString &raw)
{
    static const QRegularExpression re(QStringLiteral(R"(ffmpeg\s+version\s+([^\s,]+))"),
                                      QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(raw);
    if (match.hasMatch()) {
        return QStringLiteral("FFmpeg %1").arg(match.captured(1));
    }
    if (!raw.isEmpty()) {
        return raw.left(20);
    }
    return QStringLiteral("FFmpeg Media Suite");
}
} // namespace

MainWindow::MainWindow(LanguageManager *languageManager, QWidget *parent)
    : ElaWindow(parent)
    , m_languageManager(languageManager)
{
    QSettings settings(AppSettings::organization(), AppSettings::application());

    m_ffmpegLocator = new FFmpegLocator(this);
    m_ffmpegCaps = new FFmpegCapabilities(m_ffmpegLocator, this);
    m_commandBuilder = new FFmpegCommandBuilder(this);
    m_mediaProbe = new MediaProbeService(m_ffmpegLocator, this);
    m_recentFiles = new RecentFiles(this);
    m_jobQueue = new JobQueue(m_ffmpegLocator, m_mediaProbe, this);
    m_jobQueue->setMaxConcurrent(
        settings.value(QStringLiteral("jobs/maxConcurrent"), 1).toInt());
    m_untruncLocator = new UntruncLocator(this);

    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (geometry.isEmpty() || !restoreGeometry(geometry)) {
        resize(1200, 740);
    }
    setStackSwitchMode(ElaWindowType::StackSwitchMode::None);
    setUserInfoCardTitle(trId("app.title"));
    setUserInfoCardSubTitle(formatShortVersion(m_ffmpegLocator->ffmpegVersion()));

    connect(m_ffmpegLocator, &FFmpegLocator::pathsChanged, this, [this]() {
        setUserInfoCardSubTitle(formatShortVersion(m_ffmpegLocator->ffmpegVersion()));
        m_ffmpegCaps->scan();
    });

    initPages();
    initTrayIcon();

    connect(m_jobQueue, &JobQueue::allJobsFinished, this, [this]() {
        QSettings s(AppSettings::organization(), AppSettings::application());
        if (s.value(QStringLiteral("app/notifyOnFinish"), true).toBool()) {
            showNotification(trId("tray.allFinished.title"),
                             trId("tray.allFinished.msg"),
                             false);
        }
    });

    connect(m_jobQueue, &JobQueue::jobFinished, this, [this](const QString &name, bool success) {
        if (!success) {
            QSettings s(AppSettings::organization(), AppSettings::application());
            if (s.value(QStringLiteral("app/notifyOnFinish"), true).toBool()) {
                showNotification(trId("tray.jobFailed.title"),
                                 trId("tray.jobFailed.msg").arg(name),
                                 true);
            }
        }
    });

    // Kick off an asynchronous capability scan so startup stays responsive;
    // the codec lists only offer encoders this build ships (AI_PLAN §42).
    // Failures fall back silently.
    m_ffmpegCaps->scan();

    connect(m_languageManager, &LanguageManager::languageChanged,
            this, &MainWindow::retranslateUi);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings(AppSettings::organization(), AppSettings::application());
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());

    const bool minimizeToTray = settings.value(QStringLiteral("app/minimizeToTray"), true).toBool();
    if (minimizeToTray && m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        if (m_jobQueue && m_jobQueue->hasRunningJobs()) {
            showNotification(trId("app.title"), trId("tray.runningInBackground"), false);
        }
        event->ignore();
        return;
    }

    ElaWindow::closeEvent(event);
}

void MainWindow::initTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(windowIcon());
    m_trayIcon->setToolTip(trId("app.title"));

    m_trayMenu = new QMenu(this);
    m_showAction = m_trayMenu->addAction(trId("tray.show"));
    connect(m_showAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
    });

    m_queueAction = m_trayMenu->addAction(trId("nav.queue"));
    connect(m_queueAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
        if (m_queuePage) {
            const QString key = m_queuePage->property("ElaPageKey").toString();
            if (!key.isEmpty()) navigation(key);
        }
    });

    m_trayMenu->addSeparator();

    m_quitAction = m_trayMenu->addAction(trId("tray.quit"));
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible() && !isMinimized()) {
                hide();
            } else {
                show();
                raise();
                activateWindow();
            }
        }
    });

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
        show();
        raise();
        activateWindow();
        if (m_queuePage) {
            const QString key = m_queuePage->property("ElaPageKey").toString();
            if (!key.isEmpty()) navigation(key);
        }
    });

    m_trayIcon->show();
}

void MainWindow::showNotification(const QString &title, const QString &message, bool isError)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message,
                                isError ? QSystemTrayIcon::Warning : QSystemTrayIcon::Information,
                                4000);
    }
}

void MainWindow::initPages()
{
    m_homePage = new HomePage(m_ffmpegLocator, m_ffmpegCaps, m_jobQueue, m_untruncLocator, this);
    m_convertPage = new ConvertPage(m_ffmpegLocator, m_commandBuilder, m_mediaProbe,
                                    m_ffmpegCaps, m_recentFiles, m_jobQueue, this);
    m_imagePage = new ImagePage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_joinPage = new JoinPage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_videoPage = new VideoPage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_audioPage = new AudioPage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_repairPage = new RepairPage(m_ffmpegLocator, m_mediaProbe, m_untruncLocator, this);
    m_batchPage = new BatchPage(m_ffmpegLocator, m_commandBuilder, m_ffmpegCaps, m_jobQueue,
                                this);
    m_queuePage = new QueuePage(m_jobQueue, this);
    m_settingsPage = new SettingsPage(m_ffmpegLocator, m_ffmpegCaps, m_recentFiles,
                                      m_jobQueue, m_languageManager, this);

    m_homePage->setTargetPages(m_convertPage, m_videoPage, m_repairPage, m_batchPage, m_queuePage);
    connect(m_homePage, &HomePage::navigationRequested, this, [this](QWidget *targetPage) {
        if (targetPage) {
            const QString key = targetPage->property("ElaPageKey").toString();
            if (!key.isEmpty()) {
                navigation(key);
            }
        }
    });

    addPageNode(trId("nav.home"), m_homePage, ElaIconType::House);
    addPageNode(trId("nav.convert"), m_convertPage, ElaIconType::ArrowsRotate);
    addPageNode(trId("nav.video"), m_videoPage, ElaIconType::Video);
    addPageNode(trId("nav.audio"), m_audioPage, ElaIconType::Music);
    addPageNode(trId("nav.image"), m_imagePage, ElaIconType::Image);
    addPageNode(trId("image.join"), m_joinPage, ElaIconType::Link);
    addPageNode(trId("nav.repair"), m_repairPage, ElaIconType::Wrench);
    addPageNode(trId("nav.batch"), m_batchPage, ElaIconType::Box);
    addPageNode(trId("nav.queue"), m_queuePage, ElaIconType::List);
    addFooterNode(trId("nav.settings"), m_settingsPage, m_settingsKey, 0, ElaIconType::GearComplex);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(trId("app.title"));
    setUserInfoCardTitle(trId("app.title"));
    setUserInfoCardSubTitle(formatShortVersion(m_ffmpegLocator->ffmpegVersion()));

    setNavigationNodeTitle(m_homePage->property("ElaPageKey").toString(), trId("nav.home"));
    setNavigationNodeTitle(m_convertPage->property("ElaPageKey").toString(), trId("nav.convert"));
    setNavigationNodeTitle(m_videoPage->property("ElaPageKey").toString(), trId("nav.video"));
    setNavigationNodeTitle(m_audioPage->property("ElaPageKey").toString(), trId("nav.audio"));
    setNavigationNodeTitle(m_imagePage->property("ElaPageKey").toString(), trId("nav.image"));
    setNavigationNodeTitle(m_joinPage->property("ElaPageKey").toString(), trId("image.join"));
    setNavigationNodeTitle(m_repairPage->property("ElaPageKey").toString(), trId("nav.repair"));
    setNavigationNodeTitle(m_batchPage->property("ElaPageKey").toString(), trId("nav.batch"));
    setNavigationNodeTitle(m_queuePage->property("ElaPageKey").toString(), trId("nav.queue"));
    setNavigationNodeTitle(m_settingsKey, trId("nav.settings"));

    m_homePage->retranslateUi();
    m_convertPage->retranslateUi();
    m_videoPage->retranslateUi();
    m_audioPage->retranslateUi();
    m_imagePage->retranslateUi();
    m_joinPage->retranslateUi();
    m_repairPage->retranslateUi();
    m_batchPage->retranslateUi();
    m_queuePage->retranslateUi();
    m_settingsPage->retranslateUi();

    if (m_trayIcon) {
        m_trayIcon->setToolTip(trId("app.title"));
    }
    if (m_showAction) {
        m_showAction->setText(trId("tray.show"));
    }
    if (m_queueAction) {
        m_queueAction->setText(trId("nav.queue"));
    }
    if (m_quitAction) {
        m_quitAction->setText(trId("tray.quit"));
    }
}
