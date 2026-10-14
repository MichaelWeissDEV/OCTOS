#ifndef COMPILEROUTPUTPANE_H
#define COMPILEROUTPUTPANE_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "../core/CompilerEnums.h"

class CodeEditor;
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
    void setRawAssembly(const QString& raw) { m_rawAssembly = raw; }
    QString getRawAssembly() const { return m_rawAssembly; }

    int getCurrentJobId() const { return m_currentJobId; }
    void setCurrentJobId(int jobId) { m_currentJobId = jobId; }

    void setStatus(const QString& status, bool isError = false);
    void setSelectedCompiler(const QString& compilerName);
    void setLanguage(Language lang);

signals:
    void closeRequested();
    void compilerChanged(const QString& compiler);
    void flagsChanged(const QString& flags);
    void languageChanged(Language lang);
    void syntaxChanged(SyntaxFlavor flavor);

private slots:
    void onCompilerChanged(int index);
    void onFlagsChanged();
    void onLanguageChanged(int index);
    void onSyntaxChanged(int index);
    void onCloseClicked();

private:
    void setupUi();
    void updateTitle();
    void updateCompatibilityState();

    DockerCompilerManager* m_dockerManager;

    QWidget* m_headerWidget;
    QComboBox* m_languageCombo;
    QComboBox* m_syntaxCombo;
    QComboBox* m_compilerCombo;
    QLineEdit* m_flagsInput;
    QLabel* m_titleLabel;
    QLabel* m_statusLabel;
    QPushButton* m_closeButton;
    CodeEditor* m_editor;

    quint64 m_paneId;
    int m_currentJobId;
    Language m_currentLanguage;
    SyntaxFlavor m_currentSyntax;
    QMap<int, QVector<int>> m_lineMapping;
    QString m_rawAssembly;

    static quint64 s_nextPaneId;
};

#endif // COMPILEROUTPUTPANE_H
