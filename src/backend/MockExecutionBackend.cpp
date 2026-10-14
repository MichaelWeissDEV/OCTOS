#include "MockExecutionBackend.h"
#include <QMutexLocker>
#include <QThread>

MockExecutionBackend::MockExecutionBackend() = default;

CompilationResult MockExecutionBackend::execute(const CompilationRequest& req,
                                                const std::function<bool()>& isCancelled) {
    {
        QMutexLocker locker(&m_mutex);
        m_executedCount++;
    }

    if (m_delayMs > 0) {
        int chunks = m_delayMs / 10;
        for (int i = 0; i < chunks; ++i) {
            if (isCancelled && isCancelled()) break;
            {
                QMutexLocker locker(&m_mutex);
                if (m_cancelledJobs.contains(req.jobId)) break;
            }
            QThread::msleep(10);
        }
    }

    CompilationResult result;
    result.jobId = req.jobId;
    result.paneId = req.paneId;
    result.revision = req.revision;

    bool cancelled = (isCancelled && isCancelled());
    {
        QMutexLocker locker(&m_mutex);
        if (m_cancelledJobs.contains(req.jobId)) {
            cancelled = true;
        }
    }

    if (cancelled || req.sourceCode.contains("// TRIGGER_CANCEL")) {
        result.state = JobState::Cancelled;
        result.exitCode = -1;
        result.errorOutput = "Compilation cancelled";
        return result;
    }

    if (m_forceTimeout || req.sourceCode.contains("// TRIGGER_TIMEOUT")) {
        result.state = JobState::TimedOut;
        result.exitCode = -2;
        result.errorOutput = "Compilation timed out";
        return result;
    }

    if (m_forceError || req.sourceCode.contains("// TRIGGER_ERROR")) {
        result.state = JobState::Failed;
        result.exitCode = 1;
        result.errorOutput = "source.cpp:5:10: error: 'x' was not declared in this scope\n    return x;\n           ^";
        Diagnostic d;
        d.fileName = "source.cpp";
        d.line = 5;
        d.column = 10;
        d.severity = DiagnosticSeverity::Error;
        d.message = "'x' was not declared in this scope";
        result.diagnostics.append(d);
        return result;
    }

    result.state = JobState::Completed;
    result.exitCode = 0;

    if (req.language == Language::Java) {
        result.assemblyOutput =
            "public class Main {\n"
            "  public Main();\n"
            "    Code:\n"
            "       0: aload_0\n"
            "       1: invokespecial #1 // Method java/lang/Object.\"<init>\":()V\n"
            "       4: return\n"
            "}\n";
    } else if (req.language == Language::Python) {
        result.assemblyOutput =
            "; CPython Bytecode Disassembly (Not native machine assembly)\n"
            "  1           0 LOAD_CONST               0 (42)\n"
            "              2 STORE_NAME               0 (answer)\n"
            "              4 LOAD_CONST               1 (None)\n"
            "              6 RETURN_VALUE\n";
    } else {
        result.assemblyOutput =
            "\t.file\t\"source.cpp\"\n"
            "\t.text\n"
            "\t.globl\tmain\n"
            "\t.type\tmain, @function\n"
            "main:\n"
            ".LFB0:\n"
            "\t.cfi_startproc\n"
            "\t.loc 1 1 0\n"
            "\tpushq\t%rbp\n"
            "\t.cfi_def_cfa_offset 16\n"
            "\t.cfi_offset 6, -16\n"
            "\tmovq\t%rsp, %rbp\n"
            "\t.cfi_def_cfa_register 6\n"
            "\t.loc 1 2 0\n"
            "\tmovl\t$0, %eax\n"
            "\tpopq\t%rbp\n"
            "\t.cfi_def_cfa_offset 8\n"
            "\tret\n"
            "\t.cfi_endproc\n"
            ".LFE0:\n";
    }

    return result;
}

void MockExecutionBackend::cancel(quint64 jobId) {
    QMutexLocker locker(&m_mutex);
    m_cancelledJobs.insert(jobId);
}

int MockExecutionBackend::executedJobCount() const {
    QMutexLocker locker(&m_mutex);
    return m_executedCount;
}

bool MockExecutionBackend::wasJobCancelled(quint64 jobId) const {
    QMutexLocker locker(&m_mutex);
    return m_cancelledJobs.contains(jobId);
}
