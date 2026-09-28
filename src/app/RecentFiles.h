#pragma once

#include <QObject>
#include <QStringList>

class RecentFiles : public QObject
{
    Q_OBJECT

public:
    explicit RecentFiles(QObject *parent = nullptr);

    QStringList files() const;
    void add(const QString &path);
    void clear();

    // Pure list update: drops duplicates, moves the path to the front and
    // keeps at most maxCount entries. Empty paths (or a non-positive cap)
    // leave the list unchanged. Exact-string dedupe: paths come from the
    // same dialog and QSettings round-trips them verbatim.
    static QStringList pushFront(const QStringList &current, const QString &path,
                                 int maxCount);

    static constexpr int kMaxCount = 10;

signals:
    void changed();

private:
    void save();

    QStringList m_files;
};
