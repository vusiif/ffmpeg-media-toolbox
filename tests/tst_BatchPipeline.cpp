#include <QtTest>

#include "pipeline/BatchPipeline.h"
#include "pipeline/PipelineManager.h"

class BatchPipelineTest : public QObject
{
    Q_OBJECT

private slots:
    void linearVideoFilterGeneration();
    void audioFilterGeneration();
    void commandGenerationStandard();
    void commandGenerationWithWatermark();
    void commandGenerationAudioOnly();
    void serializationRoundTrip();
    void pipelineManagerTemplates();
};

void BatchPipelineTest::linearVideoFilterGeneration()
{
    PipelineStep step;
    step.enableCrop = true;
    step.cropX = 10;
    step.cropY = 20;
    step.cropW = 1280;
    step.cropH = 720;

    step.enableScale = true;
    step.scaleWidth = 1920;
    step.scaleHeight = -2;

    step.enableFps = true;
    step.fps = 60;

    BatchPipeline pipeline(QStringLiteral("Test"), step);
    const QString filter = pipeline.buildVideoFilter();
    QCOMPARE(filter, QStringLiteral("crop=1280:720:10:20,scale=1920:-2,fps=60"));
}

void BatchPipelineTest::audioFilterGeneration()
{
    PipelineStep step;
    step.enableAudio = true;
    step.volumeFactor = 1.5;
    step.loudnorm = true;

    BatchPipeline pipeline(QStringLiteral("TestAudio"), step);
    const QString filter = pipeline.buildAudioFilter();
    QCOMPARE(filter, QStringLiteral("volume=1.50,loudnorm=I=-16:TP=-1.5:LRA=11"));

    step.mute = true;
    pipeline.setSettings(step);
    QVERIFY(pipeline.buildAudioFilter().isEmpty());
}

void BatchPipelineTest::commandGenerationStandard()
{
    PipelineStep step;
    step.enableTrim = true;
    step.startSec = 5.0;
    step.endSec = 25.0;
    step.enableScale = true;
    step.scaleWidth = 1280;
    step.scaleHeight = -2;
    step.videoCodec = QStringLiteral("H.264");
    step.crf = 22;
    step.audioCodec = QStringLiteral("AAC");
    step.audioBitrate = QStringLiteral("128k");

    BatchPipeline pipeline(QStringLiteral("Standard"), step);
    const QStringList args = pipeline.buildCommand(QStringLiteral("input.mkv"), QStringLiteral("output.mp4"));

    QVERIFY(args.contains(QStringLiteral("-ss")));
    QVERIFY(args.contains(QStringLiteral("-t")));
    QVERIFY(args.contains(QStringLiteral("-vf")));
    const int vfIdx = args.indexOf(QStringLiteral("-vf"));
    QCOMPARE(args.at(vfIdx + 1), QStringLiteral("scale=1280:-2"));

    QVERIFY(args.contains(QStringLiteral("-c:v")));
    QCOMPARE(args.at(args.indexOf(QStringLiteral("-c:v")) + 1), QStringLiteral("libx264"));
    QVERIFY(args.contains(QStringLiteral("-crf")));
    QCOMPARE(args.at(args.indexOf(QStringLiteral("-crf")) + 1), QStringLiteral("22"));
    QVERIFY(args.contains(QStringLiteral("-c:a")));
    QCOMPARE(args.at(args.indexOf(QStringLiteral("-c:a")) + 1), QStringLiteral("aac"));
}

void BatchPipelineTest::commandGenerationWithWatermark()
{
    PipelineStep step;
    step.enableWatermark = true;
    step.watermarkPath = QStringLiteral("logo.png");
    step.watermarkPosition = QStringLiteral("top-right");
    step.watermarkOpacity = 80;
    step.watermarkMargin = 15;

    BatchPipeline pipeline(QStringLiteral("WatermarkTest"), step);
    const QStringList args = pipeline.buildCommand(QStringLiteral("in.mp4"), QStringLiteral("out.mp4"));

    // Watermark adds second input
    QVERIFY(args.contains(QStringLiteral("-i")));
    QCOMPARE(args.count(QStringLiteral("-i")), 2);
    QVERIFY(args.contains(QStringLiteral("logo.png")));

    // Watermark uses filter_complex with overlay and colorchannelmixer
    QVERIFY(args.contains(QStringLiteral("-filter_complex")));
    const QString complex = args.at(args.indexOf(QStringLiteral("-filter_complex")) + 1);
    QVERIFY(complex.contains(QStringLiteral("colorchannelmixer=aa=0.80")));
    QVERIFY(complex.contains(QStringLiteral("overlay=")));
    QVERIFY(args.contains(QStringLiteral("-map")));
    QVERIFY(args.contains(QStringLiteral("[vout]")));
}

void BatchPipelineTest::commandGenerationAudioOnly()
{
    PipelineStep step;
    step.enableAudio = true;
    step.volumeFactor = 2.0;
    step.loudnorm = true;
    step.audioCodec = QStringLiteral("MP3");
    step.audioBitrate = QStringLiteral("320k");

    BatchPipeline pipeline(QStringLiteral("AudioOnly"), step);
    const QStringList args = pipeline.buildCommand(QStringLiteral("video.mp4"), QStringLiteral("output.mp3"));

    QVERIFY(args.contains(QStringLiteral("-vn")));
    QVERIFY(!args.contains(QStringLiteral("-vf")));
    QVERIFY(args.contains(QStringLiteral("-af")));
    QVERIFY(args.contains(QStringLiteral("-c:a")));
    QCOMPARE(args.at(args.indexOf(QStringLiteral("-c:a")) + 1), QStringLiteral("libmp3lame"));
    QCOMPARE(args.at(args.indexOf(QStringLiteral("-b:a")) + 1), QStringLiteral("320k"));
}

void BatchPipelineTest::serializationRoundTrip()
{
    PipelineStep step;
    step.enableTrim = true;
    step.startSec = 10.0;
    step.endSec = 60.0;
    step.enableCrop = true;
    step.cropX = 5;
    step.cropY = 10;
    step.cropW = 1920;
    step.cropH = 800;
    step.enableScale = true;
    step.scaleWidth = 1920;
    step.scaleHeight = 800;
    step.enableFps = true;
    step.fps = 30;
    step.enableWatermark = true;
    step.watermarkPath = QStringLiteral("wm.png");
    step.watermarkPosition = QStringLiteral("bottom-left");
    step.watermarkOpacity = 50;
    step.enableAudio = true;
    step.volumeFactor = 1.25;
    step.loudnorm = true;
    step.videoCodec = QStringLiteral("H.265 / HEVC");
    step.crf = 18;

    BatchPipeline original(QStringLiteral("Custom Pipe"), step);
    const QJsonObject json = original.toJson();
    const BatchPipeline restored = BatchPipeline::fromJson(json);

    QCOMPARE(restored.name(), QStringLiteral("Custom Pipe"));
    const PipelineStep s = restored.settings();
    QCOMPARE(s.enableTrim, true);
    QCOMPARE(s.startSec, 10.0);
    QCOMPARE(s.endSec, 60.0);
    QCOMPARE(s.enableCrop, true);
    QCOMPARE(s.cropW, 1920);
    QCOMPARE(s.cropH, 800);
    QCOMPARE(s.enableScale, true);
    QCOMPARE(s.scaleWidth, 1920);
    QCOMPARE(s.scaleHeight, 800);
    QCOMPARE(s.enableFps, true);
    QCOMPARE(s.fps, 30);
    QCOMPARE(s.enableWatermark, true);
    QCOMPARE(s.watermarkPath, QStringLiteral("wm.png"));
    QCOMPARE(s.watermarkPosition, QStringLiteral("bottom-left"));
    QCOMPARE(s.watermarkOpacity, 50);
    QCOMPARE(s.enableAudio, true);
    QCOMPARE(s.volumeFactor, 1.25);
    QCOMPARE(s.loudnorm, true);
    QCOMPARE(s.videoCodec, QStringLiteral("H.265 / HEVC"));
    QCOMPARE(s.crf, 18);
}

void BatchPipelineTest::pipelineManagerTemplates()
{
    PipelineManager manager;
    const QStringList names = manager.names();
    QVERIFY(names.size() >= 5);

    // Verify built-in templates
    const QString firstBuiltIn = names.first();
    QVERIFY(manager.isBuiltIn(firstBuiltIn));

    // Built-in deletion protection
    manager.remove(firstBuiltIn);
    QVERIFY(manager.names().contains(firstBuiltIn));

    // Custom template save & load
    PipelineStep customStep;
    customStep.enableScale = true;
    customStep.scaleWidth = 640;
    customStep.scaleHeight = 360;
    BatchPipeline custom(QStringLiteral("My Custom Template"), customStep);

    manager.save(QStringLiteral("My Custom Template"), custom);
    QVERIFY(manager.names().contains(QStringLiteral("My Custom Template")));
    QVERIFY(!manager.isBuiltIn(QStringLiteral("My Custom Template")));

    const BatchPipeline loaded = manager.load(QStringLiteral("My Custom Template"));
    QCOMPARE(loaded.name(), QStringLiteral("My Custom Template"));
    QCOMPARE(loaded.settings().scaleWidth, 640);
    QCOMPARE(loaded.settings().scaleHeight, 360);

    // Clean up
    manager.remove(QStringLiteral("My Custom Template"));
    QVERIFY(!manager.names().contains(QStringLiteral("My Custom Template")));
}

QTEST_GUILESS_MAIN(BatchPipelineTest)
#include "tst_BatchPipeline.moc"
