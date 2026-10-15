#ifndef RESULTPRESENTATION_H
#define RESULTPRESENTATION_H

#include <QString>
#include "CompilationResult.h"

// How a CompilationResult is shown to the user. Kept free of widgets so it can be unit-tested:
// the UI only renders what this decides (status text, severity, notice).
enum class PresentationLevel { Ok, Warning, Error };

struct ResultPresentation {
    QString status;                     // short badge, e.g. "Docker error"
    PresentationLevel level{PresentationLevel::Ok};
    QString notice;                     // multi-line note under the header, empty = none
    PresentationLevel noticeLevel{PresentationLevel::Warning};
};

inline ResultPresentation presentResult(const CompilationResult& r) {
    ResultPresentation p;

    QStringList notes = r.warnings;
    // Truncation is surfaced even if no backend wrote a warning text for it.
    auto addOnce = [&notes](const QString& n) {
        for (const QString& e : notes) if (e.contains(n.left(24), Qt::CaseInsensitive)) return;
        notes << n;
    };
    if (r.artifactTruncated) addOnce("The output artifact was truncated and is incomplete.");
    if (r.stdoutTruncated) addOnce("Compiler stdout was truncated.");
    if (r.stderrTruncated) addOnce("Compiler stderr was truncated.");

    switch (r.state) {
        case JobState::Cancelled:
            p.status = "Cancelled";
            break;
        case JobState::TimedOut:
            p.status = "Timeout";
            p.level = PresentationLevel::Error;
            break;
        case JobState::Completed:
            if (r.success()) {
                if (r.isComplete()) { p.status = "Success"; }
                else { p.status = "Incomplete"; p.level = PresentationLevel::Warning; }
                break;
            }
            [[fallthrough]];
        default:
            p.level = PresentationLevel::Error;
            switch (r.failureOrigin) {
                case FailureOrigin::Compiler: p.status = "Compiler error"; break;
                case FailureOrigin::Docker: p.status = "Docker error"; break;
                case FailureOrigin::Limits: p.status = "Limit exceeded"; break;
                case FailureOrigin::Internal: p.status = "OCTOS internal error"; break;
                case FailureOrigin::None: p.status = "Failed"; break;
            }
            break;
    }

    p.notice = notes.join('\n');
    p.noticeLevel = (p.level == PresentationLevel::Error) ? PresentationLevel::Error : PresentationLevel::Warning;
    return p;
}

#endif // RESULTPRESENTATION_H
