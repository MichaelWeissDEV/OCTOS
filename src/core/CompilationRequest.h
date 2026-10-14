#ifndef COMPILATIONREQUEST_H
#define COMPILATIONREQUEST_H

#include <QString>
#include <QStringList>
#include "CompilerEnums.h"

struct CompilationRequest {
    quint64 jobId{0};
    quint64 paneId{0};
    quint64 revision{0};
    QString sourceCode;
    QString compilerImage;
    Language language{Language::Cpp};
    SyntaxFlavor syntaxFlavor{SyntaxFlavor::Intel};
    QString userArgs;
    int timeoutMs{30000};
};

#endif // COMPILATIONREQUEST_H
