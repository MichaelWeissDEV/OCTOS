#ifndef STRATEGYUTILS_H
#define STRATEGYUTILS_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "../core/CompilationRequest.h"
#include "../core/Diagnostic.h"

namespace StrategyUtils {

QStringList splitCompilerArguments(const QString& flags);

QStringList sanitizeUserFlags(const QStringList& rawFlags,
                              const QStringList& customForbidden = QStringList());

QVector<Diagnostic> parseClangGccDiagnostics(const QString& text);

// C and C++ file name suffixes GCC and Clang recognise without an explicit -x option.
bool isSourceSuffixFor(Language language, const QString& fileName);

// Project mode: the sandbox directory that corresponds to the project root. The main file sits at
// <root>/<directory of mainFilePath>/<name>, so the root is inputFilePath with as many trailing path
// components removed as mainFilePath has. Without a main file path: the directory of inputFilePath.
QString containerProjectRoot(const QString& inputFilePath, const QString& mainFilePath);

// C/C++ project mode: -I flags for the project root, the main file's directory and the target's include
// paths (unsafe entries are skipped). Empty in single-file mode.
QStringList projectIncludeFlags(const CompilationRequest& req, const QString& inputFilePath);

} // namespace StrategyUtils

#endif // STRATEGYUTILS_H
