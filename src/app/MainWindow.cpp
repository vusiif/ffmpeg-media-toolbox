#include "MainWindow.h"

#include "i18n/TrId.h"

#include <QCloseEvent>
#include <QSettings>

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
    ElaWindow::closeEvent(event);
}

void MainWindow::initPages()
{
    m_homePage = new HomePage(m_ffmpegLocator, m_ffmpegCaps, m_jobQueue, m_untruncLocator, this);
    m_convertPage = new ConvertPage(m_ffmpegLocator, m_commandBuilder, m_mediaProbe,
                                    m_ffmpegCaps, m_recentFiles, m_jobQueue, this);
    m_imagePage = new ImagePage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_joinPage = new JoinPage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
    m_videoPage = new VideoPage(m_ffmpegLocator, m_commandBuilder, m_jobQueue, this);
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
    addPageNode(trId("nav.image"), m_imagePage, ElaIconType::Image);
    addPageNode(trId("image.join"), m_joinPage, ElaIconType::Link);
    addPageNode(trId("nav.video"), m_videoPage, ElaIconType::Video);
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
    setNavigationNodeTitle(m_imagePage->property("ElaPageKey").toString(), trId("nav.image"));
    setNavigationNodeTitle(m_joinPage->property("ElaPageKey").toString(), trId("image.join"));
    setNavigationNodeTitle(m_videoPage->property("ElaPageKey").toString(), trId("nav.video"));
    setNavigationNodeTitle(m_repairPage->property("ElaPageKey").toString(), trId("nav.repair"));
    setNavigationNodeTitle(m_batchPage->property("ElaPageKey").toString(), trId("nav.batch"));
    setNavigationNodeTitle(m_queuePage->property("ElaPageKey").toString(), trId("nav.queue"));
    setNavigationNodeTitle(m_settingsKey, trId("nav.settings"));

    m_homePage->retranslateUi();
    m_convertPage->retranslateUi();
    m_imagePage->retranslateUi();
    m_joinPage->retranslateUi();
    m_videoPage->retranslateUi();
    m_repairPage->retranslateUi();
    m_batchPage->retranslateUi();
    m_queuePage->retranslateUi();
    m_settingsPage->retranslateUi();
}
