#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <QString>

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error,
    Note
};

struct Diagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    QString fileName;
    int line{0};
    int column{0};
    QString message;

    QString toString() const {
        QString sevStr;
        switch (severity) {
            case DiagnosticSeverity::Info:    sevStr = "info"; break;
            case DiagnosticSeverity::Warning: sevStr = "warning"; break;
            case DiagnosticSeverity::Error:   sevStr = "error"; break;
            case DiagnosticSeverity::Note:    sevStr = "note"; break;
        }
        if (line > 0) {
            return QString("%1:%2:%3: %4: %5").arg(fileName).arg(line).arg(column).arg(sevStr, message);
        }
        return QString("%1: %2").arg(sevStr, message);
    }
};

#endif // DIAGNOSTIC_H
