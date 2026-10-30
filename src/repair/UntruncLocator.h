#pragma once

#include <QObject>
#include <QString>

class UntruncLocator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(QString untruncPath READ untruncPath NOTIFY pathChanged)

public:
    explicit UntruncLocator(QObject *parent = nullptr);

    bool scan();
    void scanAsync();
    bool waitForScan(int timeoutMs = 5000);

    bool isAvailable() const;
    QString untruncPath() const;
    void setCustomPath(const QString &path);

    static QString findUntruncExecutable();

signals:
    void availabilityChanged(bool available);
    void pathChanged(const QString &path);

private:
    QString m_untruncPath;
    bool m_available = false;
};
