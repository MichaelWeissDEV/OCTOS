#ifndef STRATEGYUTILS_H
#define STRATEGYUTILS_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "../core/Diagnostic.h"

namespace StrategyUtils {

QStringList splitCompilerArguments(const QString& flags);

QStringList sanitizeUserFlags(const QStringList& rawFlags,
                              const QStringList& customForbidden = QStringList());

QVector<Diagnostic> parseClangGccDiagnostics(const QString& text);

} // namespace StrategyUtils

#endif // STRATEGYUTILS_H
