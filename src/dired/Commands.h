#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace dired {
class DirModel;
struct DirEntry;
} // namespace dired

class DiredBuffer;
class Minibuffer;

// Implements the dired commands that the key map dispatches to.
class Commands : public QObject {
    Q_OBJECT
public:
    Commands(dired::DirModel* model, DiredBuffer* buffer, Minibuffer* minibuffer, QWidget* window,
             QObject* parent = nullptr);

    void run(const QString& command);

    // Load "dir" into the listing, refreshing the buffer and window title.
    bool loadDirectory(const QString& dir);

    // Re-read the current directory (used after the embedded editor closes).
    void refresh();

    // Called to open a file in the embedded editor.
    void setEditFileCallback(std::function<void(const QString&)> callback) {
        editFile_ = std::move(callback);
    }

private:
    void bind(const QString& name, std::function<void()> fn);

    const dired::DirEntry* currentEntry() const;
    QString currentName() const;
    QStringList operationTargets() const;

    void reload(bool keepPosition = true);
    QString statusLine() const;
    void updateEcho();

    static QString expandUser(const QString& path);

    void doFindFile();
    void doUpDirectory();
    void doFindDirectory();
    void doMark();
    void doUnmark();
    void doUnmarkBackward();
    void doUnmarkAll();
    void doToggleMarks();
    void doFlagDeletion();
    void doFlaggedDelete();
    void doDeleteImmediate();
    void doCopy();
    void doRename();
    void doCreateDirectory();
    void doSortToggle();
    void doHelp();

    dired::DirModel* model_;
    DiredBuffer* buffer_;
    Minibuffer* minibuffer_;
    QWidget* window_;
    std::function<void(const QString&)> editFile_;
    QHash<QString, std::function<void()>> table_;
};
