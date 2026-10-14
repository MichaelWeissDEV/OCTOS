#ifndef COMPILERDEFINITION_H
#define COMPILERDEFINITION_H

#include <QString>
#include <QStringList>
#include "CompilerEnums.h"

struct CompilerDefinition {
    QString id;
    QString displayName;
    Language language{Language::Cpp};
    QString defaultImage;
    bool supportsIntelSyntax{true};
    bool supportsAttSyntax{true};
    bool isPartialOrExperimental{false};
    QString statusMessage;
};

#endif // COMPILERDEFINITION_H
