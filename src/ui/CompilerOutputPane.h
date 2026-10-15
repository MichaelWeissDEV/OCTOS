#ifndef COMPILEROUTPUTPANE_H
#define COMPILEROUTPUTPANE_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "../core/CompilerEnums.h"
#include "../core/CompilationResult.h"

class CodeEditor;
class AsmHighlighter;
class ArtifactHighlighter;
class DockerCompilerManager;

class CompilerOutputPane : public QWidget {
    Q_OBJECT

public:
    explicit CompilerOutputPane(DockerCompilerManager* dockerManager, QWidget* parent = nullptr);
    ~CompilerOutputPane() override = default;

    quint64 paneId() const { return m_paneId; }

    QString getSelectedCompiler() const;
    QString getCompilerFlags() const;
    Language getLanguage() const;
    SyntaxFlavor getSyntaxFlavor() const;
    CodeEditor* getEditor() const { return m_editor; }
    QMap<int, QVector<int>> getLineMapping() const { return m_lineMapping; }

    void setOutputText(const QString& text, bool isError = false);
    void setCompilerList(const QStringList& compilers);
    void setCloseButtonVisible(bool visible);
    void setAssemblyLineMapping(const QMap<int, QVector<int>>& mapping);
    void setRawToDisplayMapping(const QMap<int, int>& mapping) { m_rawToDisplay = mapping; }
    // Display line (0-based) for a raw assembly line (1-based); -1 if that line is filtered out.
    int displayLineForRaw(int rawLine) const { return m_rawToDisplay.value(rawLine, -1); }
    void setRawAssembly(const QString& raw) { m_rawAssembly = raw; }
    QString getRawAssembly() const { return m_rawAssembly; }

    int getCurrentJobId() const { return m_currentJobId; }
    void setCurrentJobId(int jobId) { m_currentJobId = jobId; }

    void setStatus(const QString& status, bool isError = false);
    void setStatusWarning(const QString& status);
    // Non-fatal or explanatory note below the header (truncation, cleanup problems, ...). Empty hides it.
    void setNotice(const QString& text, bool isError = false);
    void setSelectedCompiler(const QString& compilerName);
    void setLanguage(Language lang);
    void setCompilerFlags(const QString& flags);
    // Assembly or LLVM IR. LLVM IR is only offered for Clang images and Rust; otherwise the pane is forced to Assembly.
    OutputKind getOutputKind() const;
    void setOutputKind(OutputKind kind);
    bool outputKindSelectable() const { return m_outputCombo && m_outputCombo->isEnabled(); }
    void setSyntaxFlavor(SyntaxFlavor flavor);
    bool hasCompiler(const QString& compilerName) const;

    // --- result / artifact viewer ---------------------------------------------------------
    // Stores the result and fills the artifact selector (artifacts + a structured diagnostics list).
    void setResult(const CompilationResult& result);
    const CompilationResult& result() const { return m_result; }
    // Index into result().artifacts of the selected artifact, or -1 when the diagnostics list is shown.
    int currentArtifactIndex() const;
    bool showsDiagnostics() const;
    void showDiagnosticsView();
    QComboBox* artifactSelector() const { return m_artifactCombo; }
    QTableWidget* diagnosticsTable() const { return m_diagnosticsTable; }
    // Shows text for the given artifact type with the matching highlighting (assembly, LLVM IR,
    // JVM / CPython bytecode, CIL, or plain text).
    void showArtifactText(const QString& text, ArtifactType type, bool isError = false);
    // Busy state: shows the Cancel button while a job is queued or running.
    void setBusy(bool busy);
    void applyTheme();

signals:
    void closeRequested();
    void compilerChanged(const QString& compiler);
    void flagsChanged(const QString& flags);
    void languageChanged(Language lang);
    void syntaxChanged(SyntaxFlavor flavor);
    void outputKindChanged(OutputKind kind);
    void cancelRequested();
    // The user picked another artifact in the selector.
    void artifactSelectionChanged();
    // The user double-clicked a diagnostic (line is 1-based, 0 = unknown).
    void diagnosticActivated(const QString& file, int line, int column);

private slots:
    void onCompilerChanged(int index);
    void onFlagsChanged();
    void onLanguageChanged(int index);
    void onSyntaxChanged(int index);
    void onCloseClicked();
    void onSaveArtifact();

private:
    void setupUi();
    void updateTitle();
    void updateCompatibilityState();

    DockerCompilerManager* m_dockerManager;

    QWidget* m_headerWidget;
    QComboBox* m_languageCombo;
    QComboBox* m_syntaxCombo;
    QComboBox* m_compilerCombo;
    QComboBox* m_outputCombo{nullptr};
    QLineEdit* m_flagsInput;
    QLabel* m_titleLabel;
    QLabel* m_statusLabel;
    QLabel* m_noticeLabel;
    QPushButton* m_closeButton;
    CodeEditor* m_editor;
    QStackedWidget* m_viewStack{nullptr};
    QTableWidget* m_diagnosticsTable{nullptr};
    QComboBox* m_artifactCombo{nullptr};
    QLabel* m_infoLabel{nullptr};
    QPushButton* m_cancelButton{nullptr};
    QPushButton* m_saveButton{nullptr};
    AsmHighlighter* m_asmHighlighter{nullptr};
    ArtifactHighlighter* m_artifactHighlighter{nullptr};
    CompilationResult m_result;

    quint64 m_paneId;
    int m_currentJobId;
    Language m_currentLanguage;
    SyntaxFlavor m_currentSyntax;
    QMap<int, QVector<int>> m_lineMapping;
    QMap<int, int> m_rawToDisplay;
    QString m_rawAssembly;

    static quint64 s_nextPaneId;
};

#endif // COMPILEROUTPUTPANE_H
