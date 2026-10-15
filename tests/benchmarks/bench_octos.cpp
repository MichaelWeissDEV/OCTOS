// Reproducible benchmarks (wall time + peak RSS). They are NOT part of ctest and assert nothing:
// timing thresholds are platform dependent and do not belong in functional tests.
//
//   octos_bench                run every case, each in a fresh child process (clean peak-RSS numbers)
//   octos_bench <case>         run a single case and print one result line
//   octos_bench --list         list cases
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <sys/resource.h>
#include <cstdio>
#include <functional>
#include <map>
#include "utils/AssemblyDiffProcessor.h"
#include "parser/AssemblyParser.h"

namespace {

double peakRssMB() {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
#if defined(__APPLE__)
    return ru.ru_maxrss / (1024.0 * 1024.0);   // bytes on macOS
#else
    return ru.ru_maxrss / 1024.0;              // kilobytes on Linux
#endif
}

QString lineSet(const QString& prefix, int n, int mod, int skipEvery = 0) {
    QStringList l;
    l.reserve(n);
    for (int i = 0; i < n; ++i) {
        if (skipEvery && i % skipEvery == 0) { l << QString("\tsubl $%1, %%eax").arg(i); continue; }
        l << QString("\t%1 $%2, %%eax").arg(prefix).arg(i % mod);
    }
    return l.join('\n');
}

struct Result { double ms; QString detail; };

Result diffCase(int n, bool similar) {
    QString a = lineSet("addl", n, similar ? n : 1000003);
    QString b = similar ? lineSet("addl", n, n, 50) : lineSet("movl", n, 1000003);
    DiffOptions opt;
    opt.maxDiffSteps = 2000000000;
    opt.maxLines = 1000000;
    QElapsedTimer t; t.start();
    DiffResult r = AssemblyDiffProcessor::computeDiff(a, b, opt);
    const double ms = t.nsecsElapsed() / 1e6;
    return {ms, QString("entries=%1 +%2 -%3 complete=%4").arg(r.entries.size()).arg(r.additions).arg(r.deletions).arg(r.isComplete())};
}

QString syntheticAsm(int functions) {
    QFile f(QString(OCTOS_TEST_DATA_DIR) + "/asm/clang_x86_64_att_O3.s");
    if (!f.open(QIODevice::ReadOnly)) return QString();
    const QString base = QString::fromUtf8(f.readAll());
    QString out;
    for (int i = 0; i < functions; ++i) {
        QString copy = base;
        copy.replace("simple_add", QString("simple_add_%1").arg(i));
        copy.replace("if_else", QString("if_else_%1").arg(i));
        copy.replace("loop_sum", QString("loop_sum_%1").arg(i));
        copy.replace("sw_call", QString("sw_call_%1").arg(i));
        copy.replace("sw", QString("sw_%1").arg(i));
        copy.replace("tail_call", QString("tail_call_%1").arg(i));
        copy.replace("call_twice", QString("call_twice_%1").arg(i));
        copy.replace("indirect", QString("indirect_%1").arg(i));
        copy.replace("uses_local", QString("uses_local_%1").arg(i));
        out += copy;
    }
    return out;
}

Result parserCase(int copies) {
    const QString text = syntheticAsm(copies);
    QElapsedTimer t; t.start();
    AssemblyModule m = AssemblyParser::parseX86_64(text);
    const double ms = t.nsecsElapsed() / 1e6;
    return {ms, QString("lines=%1 functions=%2 instructions=%3").arg(text.count('\n')).arg(m.functions.size()).arg(m.totalInstructions())};
}

const std::map<QString, std::function<Result()>>& cases() {
    static const std::map<QString, std::function<Result()>> c = {
        {"diff_1k_similar", [] { return diffCase(1000, true); }},
        {"diff_10k_similar", [] { return diffCase(10000, true); }},
        {"diff_50k_similar", [] { return diffCase(50000, true); }},
        {"diff_1k_different", [] { return diffCase(1000, false); }},
        {"diff_10k_different", [] { return diffCase(10000, false); }},
        {"diff_50k_different", [] { return diffCase(50000, false); }},
        {"parser_1k_lines", [] { return parserCase(1); }},
        {"parser_100x", [] { return parserCase(100); }},
        {"parser_1000x", [] { return parserCase(1000); }},
    };
    return c;
}

}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments().mid(1);
    QTextStream out(stdout);

    if (args.value(0) == "--list") {
        for (const auto& kv : cases()) out << kv.first << "\n";
        return 0;
    }
    if (!args.isEmpty()) {
        auto it = cases().find(args[0]);
        if (it == cases().end()) { out << "unknown case\n"; return 2; }
        const double rssBefore = peakRssMB();
        Result r = it->second();
        out << QString("%1 | %2 ms | peak RSS %3 MB (baseline %4 MB) | %5\n")
                   .arg(args[0], -20).arg(r.ms, 9, 'f', 1).arg(peakRssMB(), 7, 'f', 1).arg(rssBefore, 0, 'f', 1).arg(r.detail);
        return 0;
    }
    out << "OCTOS benchmarks (each case in its own process; build type matters - use Release)\n";
    out.flush();
    for (const auto& kv : cases()) {
        QProcess p;
        p.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        p.start(QCoreApplication::applicationFilePath(), {kv.first});
        p.waitForFinished(600000);
        out << QString::fromUtf8(p.readAllStandardOutput());
        out.flush();
    }
    return 0;
}
