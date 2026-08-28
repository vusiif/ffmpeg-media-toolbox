#include "Job.h"

Job::Job(QObject *parent)
    : QObject(parent)
    , m_id(QUuid::createUuid())
{
}

QUuid Job::id() const { return m_id; }
QString Job::name() const { return m_name; }
JobType Job::type() const { return m_type; }
JobStatus Job::status() const { return m_status; }
double Job::progress() const { return m_progress; }
QString Job::errorMessage() const { return m_errorMessage; }

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

QStringList Job::inputFiles() const { return m_inputFiles; }
QString Job::outputFile() const { return m_outputFile; }
FFmpegCommand Job::command() const { return m_command; }
double Job::totalDuration() const { return m_totalDuration; }
QList<JobLog> Job::logs() const { return m_logs; }
QDateTime Job::startTime() const { return m_startTime; }
QDateTime Job::endTime() const { return m_endTime; }

void Job::setInputFiles(const QStringList &files) { m_inputFiles = files; }
void Job::setOutputFile(const QString &file) { m_outputFile = file; }
void Job::setCommand(const FFmpegCommand &cmd) { m_command = cmd; }
void Job::setTotalDuration(double duration) { m_totalDuration = duration; }

void Job::addLog(const QString &message)
{
    m_logs.append({QDateTime::currentDateTime(), message});
    if (m_logs.size() > 10000) {
        m_logs.removeFirst();
    }
}
