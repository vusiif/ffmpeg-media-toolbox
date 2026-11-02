#include "PipelineManager.h"

#include "app/AppSettings.h"
#include "i18n/TrId.h"

#include <QJsonDocument>
#include <QSettings>

namespace {
QSettings &settings()
{
    static QSettings s(AppSettings::organization(), AppSettings::application());
    return s;
}

QString localizedOrFallback(const QString &key, const QString &fallback)
{
    const QString text = trId(key.toUtf8().constData());
    return (text == key || text.isEmpty()) ? fallback : text;
}
} // namespace

PipelineManager::PipelineManager(QObject *parent)
    : QObject(parent)
{
}

QList<PipelineManager::TemplateInfo> PipelineManager::defaultBuiltInTemplates()
{
    QList<TemplateInfo> list;

    // 1. Shorts / Vertical video
    {
        TemplateInfo info;
        info.id = QStringLiteral("pipeline.template.shorts");
        info.name = localizedOrFallback(info.id, QStringLiteral("Shorts & Reels (9:16 / 1080x1920 / 30fps)"));
        info.isBuiltIn = true;

        PipelineStep step;
        step.enableScale = true;
        step.scaleWidth = 1080;
        step.scaleHeight = 1920;
        step.enableFps = true;
        step.fps = 30;
        step.enableAudio = true;
        step.loudnorm = true;
        step.outputFormat = QStringLiteral("mp4");
        step.videoCodec = QStringLiteral("H.264");
        step.audioCodec = QStringLiteral("AAC");
        step.crf = 22;

        info.pipeline = BatchPipeline(info.name, step);
        list.append(info);
    }

    // 2. Meeting / Lecture recording
    {
        TemplateInfo info;
        info.id = QStringLiteral("pipeline.template.course");
        info.name = localizedOrFallback(info.id, QStringLiteral("Meeting & Lecture (720p / 1.5x Vol / Loudnorm)"));
        info.isBuiltIn = true;

        PipelineStep step;
        step.enableScale = true;
        step.scaleWidth = 1280;
        step.scaleHeight = -2;
        step.enableAudio = true;
        step.volumeFactor = 1.5;
        step.loudnorm = true;
        step.outputFormat = QStringLiteral("mp4");
        step.videoCodec = QStringLiteral("H.264");
        step.audioCodec = QStringLiteral("AAC");
        step.crf = 24;

        info.pipeline = BatchPipeline(info.name, step);
        list.append(info);
    }

    // 3. HQ 60fps release
    {
        TemplateInfo info;
        info.id = QStringLiteral("pipeline.template.hq60");
        info.name = localizedOrFallback(info.id, QStringLiteral("HQ Release (1080p / 60fps / HEVC / 320k Audio)"));
        info.isBuiltIn = true;

        PipelineStep step;
        step.enableScale = true;
        step.scaleWidth = 1920;
        step.scaleHeight = -2;
        step.enableFps = true;
        step.fps = 60;
        step.enableAudio = true;
        step.loudnorm = true;
        step.outputFormat = QStringLiteral("mp4");
        step.videoCodec = QStringLiteral("H.265 / HEVC");
        step.audioCodec = QStringLiteral("AAC");
        step.audioBitrate = QStringLiteral("320k");
        step.crf = 18;

        info.pipeline = BatchPipeline(info.name, step);
        list.append(info);
    }

    // 4. Skip Intro
    {
        TemplateInfo info;
        info.id = QStringLiteral("pipeline.template.skipIntro");
        info.name = localizedOrFallback(info.id, QStringLiteral("Skip Intro (Skip first 5s / High Quality)"));
        info.isBuiltIn = true;

        PipelineStep step;
        step.enableTrim = true;
        step.startSec = 5.0;
        step.outputFormat = QStringLiteral("mp4");
        step.videoCodec = QStringLiteral("H.264");
        step.audioCodec = QStringLiteral("AAC");
        step.crf = 20;

        info.pipeline = BatchPipeline(info.name, step);
        list.append(info);
    }

    // 5. Watermark preset
    {
        TemplateInfo info;
        info.id = QStringLiteral("pipeline.template.watermark");
        info.name = localizedOrFallback(info.id, QStringLiteral("1080p + Top-Right Watermark Template"));
        info.isBuiltIn = true;

        PipelineStep step;
        step.enableScale = true;
        step.scaleWidth = 1920;
        step.scaleHeight = -2;
        step.enableWatermark = true;
        step.watermarkPosition = QStringLiteral("top-right");
        step.watermarkOpacity = 80;
        step.outputFormat = QStringLiteral("mp4");
        step.videoCodec = QStringLiteral("H.264");
        step.audioCodec = QStringLiteral("AAC");
        step.crf = 22;

        info.pipeline = BatchPipeline(info.name, step);
        list.append(info);
    }

    return list;
}

QList<PipelineManager::TemplateInfo> PipelineManager::allTemplates() const
{
    QList<TemplateInfo> list = defaultBuiltInTemplates();

    settings().beginGroup(QStringLiteral("pipeline_templates"));
    const QStringList userGroups = settings().childGroups();
    for (const QString &ug : userGroups) {
        settings().beginGroup(ug);
        const QString jsonStr = settings().value(QStringLiteral("data")).toString();
        settings().endGroup();

        if (!jsonStr.isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isObject()) {
                TemplateInfo info;
                info.id = ug;
                info.name = ug;
                info.pipeline = BatchPipeline::fromJson(doc.object());
                info.isBuiltIn = false;
                list.append(info);
            }
        }
    }
    settings().endGroup();

    return list;
}

bool PipelineManager::isBuiltIn(const QString &name) const
{
    if (name.trimmed().isEmpty()) return false;
    const auto defaults = defaultBuiltInTemplates();
    for (const auto &t : defaults) {
        if (t.name == name || t.id == name) {
            return true;
        }
    }
    return false;
}

QStringList PipelineManager::names() const
{
    QStringList list;
    const auto defaults = defaultBuiltInTemplates();
    for (const auto &t : defaults) {
        list.append(t.name);
    }

    settings().beginGroup(QStringLiteral("pipeline_templates"));
    const QStringList userGroups = settings().childGroups();
    settings().endGroup();

    for (const QString &ug : userGroups) {
        if (!list.contains(ug)) {
            list.append(ug);
        }
    }
    return list;
}

void PipelineManager::save(const QString &name, const BatchPipeline &pipeline)
{
    if (name.trimmed().isEmpty()) return;

    settings().beginGroup(QStringLiteral("pipeline_templates"));
    settings().beginGroup(name.trimmed());

    const QJsonDocument doc(pipeline.toJson());
    settings().setValue(QStringLiteral("data"), QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    settings().endGroup();
    settings().endGroup();
    emit templatesChanged();
}

BatchPipeline PipelineManager::load(const QString &name) const
{
    if (name.trimmed().isEmpty()) return BatchPipeline();

    const auto defaults = defaultBuiltInTemplates();
    for (const auto &t : defaults) {
        if (t.name == name || t.id == name) {
            return t.pipeline;
        }
    }

    settings().beginGroup(QStringLiteral("pipeline_templates"));
    settings().beginGroup(name);
    const QString jsonStr = settings().value(QStringLiteral("data")).toString();
    settings().endGroup();
    settings().endGroup();

    if (!jsonStr.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isObject()) {
            return BatchPipeline::fromJson(doc.object());
        }
    }

    return BatchPipeline();
}

void PipelineManager::remove(const QString &name)
{
    if (isBuiltIn(name)) {
        return; // Built-in templates cannot be deleted
    }

    settings().beginGroup(QStringLiteral("pipeline_templates"));
    settings().remove(name);
    settings().endGroup();
    emit templatesChanged();
}
