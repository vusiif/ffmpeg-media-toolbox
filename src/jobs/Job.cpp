#include "Job.h"

#include "ffmpeg/FFmpegProcess.h"

Job::Job(QObject *parent)
    : QObject(parent)
    , m_id(QUuid::createUuid())
{
}

QUuid Job::id() const { return m_id; }
void Job::setId(const QUuid &id) { m_id = id; }
QString Job::name() const { return m_name; }
JobType Job::type() const { return m_type; }
JobStatus Job::status() const { return m_status; }
double Job::progress() const { return m_progress; }
QString Job::errorMessage() const { return m_errorMessage; }

QString Job::commandProgram() const { return m_commandProgram; }
QStringList Job::commandArguments() const { return m_commandArguments; }
double Job::totalDuration() const { return m_totalDuration; }

void Job::setName(const QString &name)
{
    if (m_name == name) return;
    m_name = name;
    emit nameChanged();
}

void Job::setType(JobType type) { m_type = type; }

void Job::setStatus(JobStatus status)
{
    if (m_status == status) return;
    m_status = status;
    if (status == JobStatus::Running && m_startTime.isNull()) {
        m_startTime = QDateTime::currentDateTime();
    }
    if (status == JobStatus::Completed || status == JobStatus::Failed || status == JobStatus::Cancelled) {
        m_endTime = QDateTime::currentDateTime();
    }
    emit statusChanged();
}

void Job::setProgress(double progress)
{
    if (qFuzzyCompare(m_progress, progress)) return;
    m_progress = progress;
    emit progressChanged();
}

void Job::setErrorMessage(const QString &message) { m_errorMessage = message; }
void Job::setTotalDuration(double duration) { m_totalDuration = duration; emit commandChanged(); }

void Job::setCommand(const QString &program, const QStringList &arguments)
{
    m_commandProgram = program;
    m_commandArguments = arguments;
    emit commandChanged();
}

QString Job::commandDisplayString() const
{
    // Preview must match what JobQueue/FFmpegProcess actually executes,
    // progress plumbing included (AI_PLAN §34).
    QStringList parts;
    parts << m_commandProgram;
    for (const QString &arg : FFmpegProcess::buildInvocationArguments(m_commandArguments)) {
        if (arg.contains(QLatin1Char(' '))) {
            parts << QStringLiteral("\"%1\"").arg(arg);
        } else {
            parts << arg;
        }
    }
    return parts.join(QLatin1Char(' '));
}

QStringList Job::inputFiles() const { return m_inputFiles; }
QString Job::outputFile() const { return m_outputFile; }
QList<JobLog> Job::logs() const { return m_logs; }
QDateTime Job::startTime() const { return m_startTime; }
QDateTime Job::endTime() const { return m_endTime; }

void Job::setInputFiles(const QStringList &files) { m_inputFiles = files; }
void Job::setOutputFile(const QString &file) { m_outputFile = file; }
void Job::setStartTime(const QDateTime &dt) { m_startTime = dt; }
void Job::setEndTime(const QDateTime &dt) { m_endTime = dt; }
void Job::setLogs(const QList<JobLog> &logs) { m_logs = logs; }

void Job::addLog(const QString &message)
{
    m_logs.append({QDateTime::currentDateTime(), message});
    if (m_logs.size() > 10000) {
        m_logs.removeFirst();
    }
}

bool Job::autoSoftwareFallback() const { return m_autoSoftwareFallback; }
void Job::setAutoSoftwareFallback(bool enable) { m_autoSoftwareFallback = enable; }
bool Job::fallbackAttempted() const { return m_fallbackAttempted; }
void Job::setFallbackAttempted(bool attempted) { m_fallbackAttempted = attempted; }

QJsonObject Job::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = m_id.toString(QUuid::WithoutBraces);
    obj[QStringLiteral("name")] = m_name;
    obj[QStringLiteral("type")] = static_cast<int>(m_type);
    obj[QStringLiteral("status")] = static_cast<int>(m_status);
    obj[QStringLiteral("progress")] = m_progress;
    obj[QStringLiteral("errorMessage")] = m_errorMessage;
    obj[QStringLiteral("commandProgram")] = m_commandProgram;

    QJsonArray args;
    for (const QString &arg : m_commandArguments) {
        args.append(arg);
    }
    obj[QStringLiteral("commandArguments")] = args;
    obj[QStringLiteral("totalDuration")] = m_totalDuration;

    QJsonArray inputs;
    for (const QString &in : m_inputFiles) {
        inputs.append(in);
    }
    obj[QStringLiteral("inputFiles")] = inputs;
    obj[QStringLiteral("outputFile")] = m_outputFile;
    obj[QStringLiteral("autoSoftwareFallback")] = m_autoSoftwareFallback;
    obj[QStringLiteral("fallbackAttempted")] = m_fallbackAttempted;

    if (m_startTime.isValid()) {
        obj[QStringLiteral("startTime")] = m_startTime.toString(Qt::ISODate);
    }
    if (m_endTime.isValid()) {
        obj[QStringLiteral("endTime")] = m_endTime.toString(Qt::ISODate);
    }

    QJsonArray logsArray;
    for (const JobLog &log : m_logs) {
        QJsonObject logObj;
        logObj[QStringLiteral("time")] = log.timestamp.toString(Qt::ISODate);
        logObj[QStringLiteral("msg")] = log.message;
        logsArray.append(logObj);
    }
    obj[QStringLiteral("logs")] = logsArray;

    return obj;
}

Job* Job::fromJson(const QJsonObject &obj, QObject *parent)
{
    auto *job = new Job(parent);

    const QString idStr = obj.value(QStringLiteral("id")).toString();
    if (!idStr.isEmpty()) {
        const QUuid id = QUuid::fromString(idStr);
        if (!id.isNull()) {
            job->setId(id);
        }
    }

    job->setName(obj.value(QStringLiteral("name")).toString());
    job->setType(static_cast<JobType>(obj.value(QStringLiteral("type")).toInt(0)));

    int statusInt = obj.value(QStringLiteral("status")).toInt(static_cast<int>(JobStatus::Pending));
    auto status = static_cast<JobStatus>(statusInt);
    QString errMsg = obj.value(QStringLiteral("errorMessage")).toString();
    // If the job was Running or Preparing when the application exited, mark as Cancelled/Interrupted
    if (status == JobStatus::Running || status == JobStatus::Preparing) {
        status = JobStatus::Cancelled;
        if (errMsg.isEmpty()) {
            errMsg = QStringLiteral("Session interrupted by application exit");
        }
    }
    job->setStatus(status);
    job->setErrorMessage(errMsg);

    job->setProgress(obj.value(QStringLiteral("progress")).toDouble(0.0));
    job->setTotalDuration(obj.value(QStringLiteral("totalDuration")).toDouble(0.0));
    job->setOutputFile(obj.value(QStringLiteral("outputFile")).toString());
    job->setAutoSoftwareFallback(obj.value(QStringLiteral("autoSoftwareFallback")).toBool(true));
    job->setFallbackAttempted(obj.value(QStringLiteral("fallbackAttempted")).toBool(false));

    QStringList inputs;
    const QJsonArray inputsArr = obj.value(QStringLiteral("inputFiles")).toArray();
    for (const auto &val : inputsArr) {
        inputs.append(val.toString());
    }
    job->setInputFiles(inputs);

    const QString prog = obj.value(QStringLiteral("commandProgram")).toString();
    QStringList args;
    const QJsonArray argsArr = obj.value(QStringLiteral("commandArguments")).toArray();
    for (const auto &val : argsArr) {
        args.append(val.toString());
    }
    job->setCommand(prog, args);

    if (obj.contains(QStringLiteral("startTime"))) {
        job->setStartTime(QDateTime::fromString(obj.value(QStringLiteral("startTime")).toString(), Qt::ISODate));
    }
    if (obj.contains(QStringLiteral("endTime"))) {
        job->setEndTime(QDateTime::fromString(obj.value(QStringLiteral("endTime")).toString(), Qt::ISODate));
    }

    QList<JobLog> logs;
    const QJsonArray logsArr = obj.value(QStringLiteral("logs")).toArray();
    for (const auto &val : logsArr) {
        const QJsonObject logObj = val.toObject();
        JobLog log;
        log.timestamp = QDateTime::fromString(logObj.value(QStringLiteral("time")).toString(), Qt::ISODate);
        log.message = logObj.value(QStringLiteral("msg")).toString();
        logs.append(log);
    }
    job->setLogs(logs);

    return job;
}
