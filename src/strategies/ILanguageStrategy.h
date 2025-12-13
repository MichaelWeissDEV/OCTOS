# 1 "./src/strategies/ILanguageStrategy.h"
#ifndef ILANGUAGESTRATEGY_H
#define ILANGUAGESTRATEGY_H

#include <QList>
#include <QMap>
#include <QString>

struct CompilationResult {
    bool success;
    QString primaryOutput;
    QString secondaryOutput;
    QString errorMessage;
    QMap<int, int> lineMapping;
};

class ILanguageStrategy {
   public:
    virtual ~ILanguageStrategy() = default;

    virtual CompilationResult compile(const QString &source, const QString &compiler,
                                      const QString &flags) = 0;

    virtual QString getHighlightingRules() = 0;

    virtual bool supportsDualView() const { return false; }

    virtual QString getLanguageName() const = 0;

    virtual QString getFileExtension() const = 0;

    virtual void cleanup() {}

    virtual QStringList getRecommendedCompilers() const = 0;

    virtual bool isDualViewActive() const { return false; }

    virtual void setDualViewMode(bool enabled) { Q_UNUSED(enabled); }
};

#endif
