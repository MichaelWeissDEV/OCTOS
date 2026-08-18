#include "CompilerOutputPane.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVariant>

#include "MainWindow.h"

Q_DECLARE_METATYPE(Language)
Q_DECLARE_METATYPE(SyntaxFlavor)

CompilerOutputPane::CompilerOutputPane(DockerCompilerManager* dockerManager, QWidget* parent)
    : QWidget(parent),
      m_dockerManager(dockerManager),
      m_currentJobId(-1),
      m_currentLanguage(Language::Cpp),
      m_currentSyntax(SyntaxFlavor::Intel) {
    setupUi();
}

void CompilerOutputPane::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_headerWidget = new QWidget();
    m_headerWidget->setStyleSheet(R"(
        QWidget {
            background-color: #3c3f41;
            border-bottom: 1px solid #2b2b2b;
        }
    )");

    QGridLayout* headerLayout = new QGridLayout(m_headerWidget);
    headerLayout->setContentsMargins(6, 4, 6, 4);
    headerLayout->setHorizontalSpacing(6);
    headerLayout->setVerticalSpacing(4);

    QLabel* langLabel = new QLabel("Language:");
    langLabel->setStyleSheet("color: #cccccc; font-size: 11px;");

    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("C++", QVariant::fromValue(Language::Cpp));
    m_languageCombo->addItem("C", QVariant::fromValue(Language::C));
    m_languageCombo->addItem("Rust", QVariant::fromValue(Language::Rust));
    m_languageCombo->addItem("C#", QVariant::fromValue(Language::CSharp));
    m_languageCombo->addItem("Java", QVariant::fromValue(Language::Java));
    m_languageCombo->addItem("Python", QVariant::fromValue(Language::Python));
    m_languageCombo->addItem("Ada", QVariant::fromValue(Language::Ada));
    m_languageCombo->setMinimumWidth(70);
    m_languageCombo->setMaximumWidth(80);
    m_languageCombo->setStyleSheet(R"(
        QComboBox {
            padding: 4px 8px;
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            color: #cccccc;
        }
        QComboBox QAbstractItemView {
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            selection-background-color: #094771;
            color: #cccccc;
        }
    )");
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onLanguageChanged);

    QLabel* syntaxLabel = new QLabel("Syntax:");
    syntaxLabel->setStyleSheet("color: #cccccc; font-size: 11px;");

    m_syntaxCombo = new QComboBox();
    m_syntaxCombo->addItem("Intel", QVariant::fromValue(SyntaxFlavor::Intel));
    m_syntaxCombo->addItem("AT&T", QVariant::fromValue(SyntaxFlavor::ATT));
    m_syntaxCombo->setMinimumWidth(60);
    m_syntaxCombo->setMaximumWidth(70);
    m_syntaxCombo->setStyleSheet(R"(
        QComboBox {
            padding: 4px 8px;
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            color: #cccccc;
        }
        QComboBox QAbstractItemView {
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            selection-background-color: #094771;
            color: #cccccc;
        }
    )");
    connect(m_syntaxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onSyntaxChanged);

    QLabel* compilerLabel = new QLabel("Compiler:");
    compilerLabel->setStyleSheet("color: #cccccc; font-size: 11px;");

    m_compilerCombo = new QComboBox();
    m_compilerCombo->setMinimumWidth(120);
    m_compilerCombo->setStyleSheet(R"(
        QComboBox {
            padding: 4px 8px;
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            color: #cccccc;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #cccccc;
            margin-right: 5px;
        }
        QComboBox QAbstractItemView {
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            selection-background-color: #094771;
            color: #cccccc;
        }
    )");

    connect(m_compilerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CompilerOutputPane::onCompilerChanged);

    QLabel* flagsLabel = new QLabel("Flags:");
    flagsLabel->setStyleSheet("color: #cccccc; font-size: 11px;");

    m_flagsInput = new QLineEdit();
    m_flagsInput->setPlaceholderText("e.g., -O3 -march=native");
    m_flagsInput->setMinimumWidth(100);
    m_flagsInput->setStyleSheet(R"(
        QLineEdit {
            padding: 4px 8px;
            background-color: #2d2d30;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            color: #cccccc;
        }
        QLineEdit:focus {
            border: 1px solid #0e639c;
        }
    )");

    connect(m_flagsInput, &QLineEdit::textChanged, this, &CompilerOutputPane::onFlagsChanged);

    m_closeButton = new QPushButton("✕");
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            color: #cccccc;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #e81123;
            border-radius: 3px;
        }
    )");

    connect(m_closeButton, &QPushButton::clicked, this, &CompilerOutputPane::onCloseClicked);

    m_titleLabel = new QLabel("Compiler Output");
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #4fc1ff; padding-bottom: 4px; border-bottom: 1px solid #3e3e42; margin-bottom: 4px;");
    headerLayout->addWidget(m_titleLabel, 0, 0, 1, 7);

    headerLayout->addWidget(langLabel, 1, 0);
    headerLayout->addWidget(m_languageCombo, 1, 1);
    headerLayout->addWidget(syntaxLabel, 1, 2);
    headerLayout->addWidget(m_syntaxCombo, 1, 3);
    headerLayout->addWidget(compilerLabel, 1, 4);
    headerLayout->addWidget(m_compilerCombo, 1, 5);
    headerLayout->addWidget(m_closeButton, 0, 6, 2, 1, Qt::AlignTop | Qt::AlignRight);

    headerLayout->addWidget(flagsLabel, 2, 0);
    headerLayout->addWidget(m_flagsInput, 2, 1, 1, 6);

    headerLayout->setColumnStretch(5, 1);

    m_editor = new CodeEditor();
    m_editor->setReadOnly(true);

    QPalette palette = m_editor->palette();
    palette.setColor(QPalette::Base, QColor("#1e1e1e"));
    palette.setColor(QPalette::Text, QColor("#d4d4d4"));
    m_editor->setPalette(palette);

    mainLayout->addWidget(m_headerWidget);
    mainLayout->addWidget(m_editor);
    updateTitle();
}

void CompilerOutputPane::setOutputText(const QString& text, bool isError) {
    m_editor->setPlainText(text);
    m_editor->setVisible(true);

    QPalette palette = m_editor->palette();
    palette.setColor(QPalette::Text, QColor("#d4d4d4"));
    palette.setColor(QPalette::Base, QColor("#1e1e1e"));
    m_editor->setPalette(palette);

    m_editor->update();
}

void CompilerOutputPane::setCompilerList(const QStringList& compilers) {
    QString current = m_compilerCombo->currentText();
    m_compilerCombo->clear();
    m_compilerCombo->addItems(compilers);

    int index = m_compilerCombo->findText(current);
    if (index >= 0) {
        m_compilerCombo->setCurrentIndex(index);
    }
    updateTitle();
}

QString CompilerOutputPane::getSelectedCompiler() const { return m_compilerCombo->currentText(); }

QString CompilerOutputPane::getCompilerFlags() const { return m_flagsInput->text(); }

Language CompilerOutputPane::getLanguage() const { return m_currentLanguage; }

SyntaxFlavor CompilerOutputPane::getSyntaxFlavor() const { return m_currentSyntax; }

void CompilerOutputPane::setCloseButtonVisible(bool visible) { m_closeButton->setVisible(visible); }

void CompilerOutputPane::setAssemblyLineMapping(const QMap<int, QVector<int>>& mapping) {
    m_lineMapping = mapping;
}

void CompilerOutputPane::onCompilerChanged(int index) {
    if (index >= 0) {
        updateTitle();
        emit compilerChanged(m_compilerCombo->currentText());
    }
}

void CompilerOutputPane::onLanguageChanged(int index) {
    if (index >= 0) {
        m_currentLanguage = m_languageCombo->itemData(index).value<Language>();
        updateTitle();
        emit languageChanged(m_currentLanguage);
    }
}

void CompilerOutputPane::onSyntaxChanged(int index) {
    if (index >= 0) {
        m_currentSyntax = m_syntaxCombo->itemData(index).value<SyntaxFlavor>();
        updateTitle();
        emit syntaxChanged(m_currentSyntax);
    }
}

void CompilerOutputPane::onFlagsChanged() { 
    updateTitle();
    emit flagsChanged(m_flagsInput->text()); 
}

void CompilerOutputPane::onCloseClicked() { emit closeRequested(); }

void CompilerOutputPane::updateTitle() {
    QString lang = m_languageCombo->currentText();
    QString comp = m_compilerCombo->currentText();
    if (comp.isEmpty()) comp = "No Compiler Selected";
    QString syn = m_syntaxCombo->currentText();
    QString flags = m_flagsInput->text();
    m_titleLabel->setText(QString("%1 | %2 | %3 | %4").arg(lang, comp, syn, flags));
}
