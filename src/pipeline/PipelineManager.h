#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "BatchPipeline.h"

class PipelineManager : public QObject
{
    Q_OBJECT

public:
    struct TemplateInfo {
        QString id;
        QString name;
        BatchPipeline pipeline;
        bool isBuiltIn = false;
    };

    explicit PipelineManager(QObject *parent = nullptr);

    QStringList names() const;
    QList<TemplateInfo> allTemplates() const;

    bool isBuiltIn(const QString &name) const;
    void save(const QString &name, const BatchPipeline &pipeline);
    BatchPipeline load(const QString &name) const;
    void remove(const QString &name);

signals:
    void templatesChanged();

private:
    static QList<TemplateInfo> defaultBuiltInTemplates();
};
