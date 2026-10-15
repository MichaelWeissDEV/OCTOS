#include "ProfileDialog.h"
#include "Theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(Language)

ProfileDialog::ProfileDialog(const ProfileStore& store, const QStringList& installedImages, QWidget* parent)
    : QDialog(parent), m_store(store) {
    setWindowTitle("Compiler Profiles");
    resize(760, 460);

    auto* root = new QHBoxLayout(this);

    auto* left = new QVBoxLayout();
    m_list = new QListWidget();
    left->addWidget(m_list, 1);
    auto* lb = new QHBoxLayout();
    auto* nw = new QPushButton("New");
    auto* dup = new QPushButton("Duplicate");
    Theme::setRole(dup, "subtle");
    auto* del = new QPushButton("Delete");
    Theme::setRole(del, "danger");
    lb->addWidget(nw);
    lb->addWidget(dup);
    lb->addWidget(del);
    left->addLayout(lb);
    root->addLayout(left, 1);

    m_form = new QWidget();
    auto* form = new QFormLayout(m_form);
    m_name = new QLineEdit();
    m_image = new QComboBox();
    m_image->setEditable(true);
    m_image->addItems(installedImages);
    m_language = new QComboBox();
    for (auto [label, lang] : {std::pair<const char*, Language>{"C++", Language::Cpp}, {"C", Language::C}, {"Rust", Language::Rust},
                               {"Java", Language::Java}, {"Python", Language::Python}, {"C#", Language::CSharp}, {"Ada", Language::Ada}})
        m_language->addItem(label, QVariant::fromValue(lang));
    m_optimization = new QComboBox();
    m_optimization->setEditable(true);
    m_optimization->addItems({"", "-O0", "-O1", "-O2", "-O3", "-Os", "-C opt-level=0", "-C opt-level=3"});
    m_flags = new QLineEdit();
    m_flags->setPlaceholderText("additional flags, e.g. -Wall -march=native");
    m_syntax = new QComboBox();
    m_syntax->addItem("Intel", int(SyntaxFlavor::Intel));
    m_syntax->addItem("AT&T", int(SyntaxFlavor::ATT));
    m_crate = new QComboBox();
    m_crate->addItem("Auto (main() → binary, otherwise library)", int(RustCrateType::Auto));
    m_crate->addItem("Binary", int(RustCrateType::Binary));
    m_crate->addItem("Library", int(RustCrateType::Library));
    m_arch = new QLineEdit();
    m_arch->setPlaceholderText("informational, e.g. x86_64");
    form->addRow("Name", m_name);
    form->addRow("Compiler image", m_image);
    form->addRow("Language", m_language);
    form->addRow("Optimization", m_optimization);
    form->addRow("Flags", m_flags);
    form->addRow("Syntax", m_syntax);
    form->addRow("Rust crate type", m_crate);
    form->addRow("Target architecture", m_arch);
    auto* hint = new QLabel("Two profiles (e.g. GCC -O0 and -O3) can be compared side by side without re-entering anything.");
    hint->setWordWrap(true);
    Theme::setRole(hint, "muted");
    form->addRow(hint);

    auto* right = new QVBoxLayout();
    right->addWidget(m_form, 1);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    right->addWidget(bb);
    root->addLayout(right, 2);

    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_list, &QListWidget::currentRowChanged, this, &ProfileDialog::onSelectionChanged);
    connect(nw, &QPushButton::clicked, this, &ProfileDialog::onNew);
    connect(dup, &QPushButton::clicked, this, &ProfileDialog::onDuplicate);
    connect(del, &QPushButton::clicked, this, &ProfileDialog::onDelete);
    connect(m_name, &QLineEdit::textEdited, this, &ProfileDialog::onFieldEdited);
    connect(m_image, &QComboBox::editTextChanged, this, &ProfileDialog::onFieldEdited);
    connect(m_language, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileDialog::onFieldEdited);
    connect(m_optimization, &QComboBox::editTextChanged, this, &ProfileDialog::onFieldEdited);
    connect(m_flags, &QLineEdit::textEdited, this, &ProfileDialog::onFieldEdited);
    connect(m_syntax, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileDialog::onFieldEdited);
    connect(m_crate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileDialog::onFieldEdited);
    connect(m_arch, &QLineEdit::textEdited, this, &ProfileDialog::onFieldEdited);

    rebuildList(QString());
}

void ProfileDialog::rebuildList(const QString& selectId) {
    m_updating = true;
    m_list->clear();
    int sel = -1;
    for (int i = 0; i < m_store.profiles().size(); ++i) {
        const auto& p = m_store.profiles()[i];
        auto* it = new QListWidgetItem(p.name.isEmpty() ? p.id : p.name);
        it->setData(Qt::UserRole, p.id);
        m_list->addItem(it);
        if (p.id == selectId) sel = i;
    }
    m_updating = false;
    if (sel < 0 && m_list->count() > 0) sel = 0;
    m_list->setCurrentRow(sel);
    if (sel < 0) fillForm(nullptr);
}

CompilerProfile* ProfileDialog::current() {
    auto* it = m_list->currentItem();
    if (!it) return nullptr;
    const QString id = it->data(Qt::UserRole).toString();
    return const_cast<CompilerProfile*>(m_store.find(id));
}

void ProfileDialog::fillForm(const CompilerProfile* p) {
    m_updating = true;
    m_form->setEnabled(p != nullptr);
    m_name->setText(p ? p->name : QString());
    m_image->setEditText(p ? p->compilerImage : QString());
    const int li = p ? m_language->findData(QVariant::fromValue(p->language)) : 0;
    m_language->setCurrentIndex(qMax(0, li));
    m_optimization->setEditText(p ? p->optimization : QString());
    m_flags->setText(p ? p->userFlags : QString());
    m_syntax->setCurrentIndex(p && p->syntaxFlavor == SyntaxFlavor::ATT ? 1 : 0);
    m_crate->setCurrentIndex(p ? m_crate->findData(int(p->rustCrateType)) : 0);
    m_arch->setText(p ? p->targetArchitecture : QString());
    m_crate->setEnabled(p && p->language == Language::Rust);
    m_updating = false;
}

void ProfileDialog::onSelectionChanged() {
    if (m_updating) return;
    fillForm(current());
}

void ProfileDialog::onFieldEdited() {
    if (m_updating) return;
    CompilerProfile* p = current();
    if (!p) return;
    p->name = m_name->text();
    p->compilerImage = m_image->currentText().trimmed();
    p->language = m_language->currentData().value<Language>();
    p->optimization = m_optimization->currentText().trimmed();
    p->userFlags = m_flags->text().trimmed();
    p->syntaxFlavor = m_syntax->currentData().toInt() == int(SyntaxFlavor::ATT) ? SyntaxFlavor::ATT : SyntaxFlavor::Intel;
    p->rustCrateType = static_cast<RustCrateType>(m_crate->currentData().toInt());
    p->targetArchitecture = m_arch->text().trimmed();
    m_crate->setEnabled(p->language == Language::Rust);
    if (auto* it = m_list->currentItem()) it->setText(p->name.isEmpty() ? p->id : p->name);
}

void ProfileDialog::onNew() {
    CompilerProfile p;
    p.name = "New profile";
    p.id = m_store.makeUniqueId(p.name);
    if (m_image->count() > 0) p.compilerImage = m_image->itemText(0);
    m_store.add(p);
    rebuildList(p.id);
    m_name->setFocus();
    m_name->selectAll();
}

void ProfileDialog::onDuplicate() {
    const CompilerProfile* src = current();
    if (!src) return;
    CompilerProfile p = *src;
    p.name += " (copy)";
    p.id = m_store.makeUniqueId(p.name);
    m_store.add(p);
    rebuildList(p.id);
}

void ProfileDialog::onDelete() {
    const CompilerProfile* src = current();
    if (!src) return;
    m_store.remove(src->id);
    rebuildList(QString());
}

CompareProfilesDialog::CompareProfilesDialog(const QVector<CompilerProfile>& profiles, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Compare Profiles");
    auto* v = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_a = new QComboBox();
    m_b = new QComboBox();
    for (const auto& p : profiles) {
        m_a->addItem(p.name.isEmpty() ? p.id : p.name, p.id);
        m_b->addItem(p.name.isEmpty() ? p.id : p.name, p.id);
    }
    if (m_b->count() > 1) m_b->setCurrentIndex(1);
    form->addRow("Profile A", m_a);
    form->addRow("Profile B", m_b);
    v->addLayout(form);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString CompareProfilesDialog::profileA() const { return m_a->currentData().toString(); }
QString CompareProfilesDialog::profileB() const { return m_b->currentData().toString(); }
