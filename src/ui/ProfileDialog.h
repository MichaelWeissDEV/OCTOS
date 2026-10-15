#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include "../core/ProfileStore.h"

// Create, edit, duplicate and delete compiler profiles (user level; they persist between sessions).
class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    ProfileDialog(const ProfileStore& store, const QStringList& installedImages, QWidget* parent = nullptr);

    ProfileStore result() const { return m_store; }
    QListWidget* list() const { return m_list; }

private slots:
    void onSelectionChanged();
    void onNew();
    void onDuplicate();
    void onDelete();
    void onFieldEdited();

private:
    void fillForm(const CompilerProfile* p);
    void rebuildList(const QString& selectId);
    CompilerProfile* current();

    ProfileStore m_store;
    QListWidget* m_list;
    QLineEdit* m_name;
    QComboBox* m_image;
    QComboBox* m_language;
    QComboBox* m_optimization;
    QLineEdit* m_flags;
    QComboBox* m_syntax;
    QComboBox* m_crate;
    QComboBox* m_output;
    QLineEdit* m_arch;
    QWidget* m_form;
    bool m_updating{false};
};

// Picks two profiles for a side-by-side comparison.
class CompareProfilesDialog : public QDialog {
    Q_OBJECT

public:
    CompareProfilesDialog(const QVector<CompilerProfile>& profiles, QWidget* parent = nullptr);
    QString profileA() const;
    QString profileB() const;

private:
    QComboBox* m_a;
    QComboBox* m_b;
};

#endif // PROFILEDIALOG_H
