#include "dired/Commands.h"

#include "dired/DiredBuffer.h"
#include "dired/DirModel.h"
#include "ui/Minibuffer.h"
#include "util/Format.h"
#include "util/FsOps.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTextCursor>
#include <QWidget>

#include <utility>

using dired::DirModel;
using dired::DirEntry;

Commands::Commands(DirModel* model, DiredBuffer* buffer, Minibuffer* minibuffer, QWidget* window,
                   QObject* parent)
    : QObject(parent), model_(model), buffer_(buffer), minibuffer_(minibuffer), window_(window) {
    bind(QStringLiteral("next-line"), [this] { buffer_->moveLine(1); });
    bind(QStringLiteral("previous-line"), [this] { buffer_->moveLine(-1); });
    bind(QStringLiteral("forward-char"), [this] { buffer_->moveCursor(QTextCursor::Right); });
    bind(QStringLiteral("backward-char"), [this] { buffer_->moveCursor(QTextCursor::Left); });
    bind(QStringLiteral("move-beginning-of-line"),
         [this] { buffer_->moveCursor(QTextCursor::StartOfLine); });
    bind(QStringLiteral("move-end-of-line"), [this] { buffer_->moveCursor(QTextCursor::EndOfLine); });
    bind(QStringLiteral("beginning-of-buffer"), [this] {
        buffer_->gotoLine(0);
        buffer_->moveCursor(QTextCursor::StartOfLine);
    });
    bind(QStringLiteral("end-of-buffer"), [this] {
        buffer_->gotoLine(buffer_->blockCount() - 1);
        buffer_->moveCursor(QTextCursor::EndOfLine);
    });
    bind(QStringLiteral("scroll-up"), [this] { buffer_->moveLine(buffer_->visibleLineCount()); });
    bind(QStringLiteral("scroll-down"), [this] { buffer_->moveLine(-buffer_->visibleLineCount()); });
    bind(QStringLiteral("recenter"), [this] { buffer_->centerCursor(); });

    bind(QStringLiteral("dired-find-file"), [this] { doFindFile(); });
    bind(QStringLiteral("dired-up-directory"), [this] { doUpDirectory(); });
    bind(QStringLiteral("dired-find-directory"), [this] { doFindDirectory(); });

    bind(QStringLiteral("dired-mark"), [this] { doMark(); });
    bind(QStringLiteral("dired-unmark"), [this] { doUnmark(); });
    bind(QStringLiteral("dired-unmark-backward"), [this] { doUnmarkBackward(); });
    bind(QStringLiteral("dired-unmark-all-marks"), [this] { doUnmarkAll(); });
    bind(QStringLiteral("dired-toggle-marks"), [this] { doToggleMarks(); });

    bind(QStringLiteral("dired-flag-file-deletion"), [this] { doFlagDeletion(); });
    bind(QStringLiteral("dired-do-flagged-delete"), [this] { doFlaggedDelete(); });
    bind(QStringLiteral("dired-do-delete"), [this] { doDeleteImmediate(); });
    bind(QStringLiteral("dired-do-copy"), [this] { doCopy(); });
    bind(QStringLiteral("dired-do-rename"), [this] { doRename(); });
    bind(QStringLiteral("dired-create-directory"), [this] { doCreateDirectory(); });

    bind(QStringLiteral("revert-buffer"), [this] { reload(false); });
    bind(QStringLiteral("dired-sort-toggle"), [this] { doSortToggle(); });
    bind(QStringLiteral("dired-help"), [this] { doHelp(); });
    bind(QStringLiteral("keyboard-quit"), [this] { minibuffer_->message(statusLine()); });
    bind(QStringLiteral("quit-window"), [this] { window_->close(); });
    bind(QStringLiteral("save-buffers-kill-emacs"), [this] { window_->close(); });
}

void Commands::bind(const QString& name, std::function<void()> fn) {
    table_.insert(name, std::move(fn));
}

void Commands::run(const QString& command) {
    const auto it = table_.find(command);
    if (it != table_.end())
        (*it)();
    else
        minibuffer_->message(QStringLiteral("Undefined command: ") + command);
}

bool Commands::loadDirectory(const QString& dir) {
    QString error;
    if (!model_->load(dir, &error)) {
        minibuffer_->message(error);
        return false;
    }
    buffer_->refreshText(false);
    window_->setWindowTitle(QStringLiteral("dired: ") +
                            QDir::toNativeSeparators(model_->currentDir()));
    updateEcho();
    return true;
}

void Commands::reload(bool keepPosition) {
    const QString name = currentName();
    const QString dir = model_->currentDir();
    QString error;
    if (!model_->load(dir, &error)) {
        minibuffer_->message(error);
        return;
    }
    buffer_->refreshText(false);
    if (keepPosition) {
        const int idx = model_->indexOfName(name);
        if (idx >= 0)
            buffer_->gotoLine(idx);
    }
    updateEcho();
}

const DirEntry* Commands::currentEntry() const {
    const int line = buffer_->currentLine();
    if (line < 0 || line >= model_->count())
        return nullptr;
    return &model_->at(line);
}

QString Commands::currentName() const {
    const DirEntry* e = currentEntry();
    return e ? e->name : QString();
}

QStringList Commands::operationTargets() const {
    QStringList marked = model_->markedNames();
    if (!marked.isEmpty())
        return marked;
    const DirEntry* e = currentEntry();
    if (e && !e->isDotEntry())
        return {e->name};
    return {};
}

QString Commands::statusLine() const {
    int files = 0;
    qint64 total = 0;
    for (int i = 2; i < model_->count(); ++i) {
        ++files;
        total += model_->at(i).size;
    }
    return QStringLiteral("%1  files:%2  total:%3  marked:%4  sort:%5")
        .arg(QDir::toNativeSeparators(model_->currentDir()))
        .arg(files)
        .arg(Format::humanSize(total))
        .arg(model_->markedCount())
        .arg(model_->sortModeName());
}

void Commands::updateEcho() {
    if (!minibuffer_->isBusy())
        minibuffer_->message(statusLine());
}

QString Commands::expandUser(const QString& path) {
    if (path.startsWith(QLatin1Char('~')))
        return QDir::homePath() + path.mid(1);
    return path;
}

QString Commands::editorCommand() const {
    QString e = qEnvironmentVariable("EDITOR");
    if (e.isEmpty())
        e = qEnvironmentVariable("VISUAL");
    if (e.isEmpty()) {
        QSettings settings;
        e = settings.value(QStringLiteral("editor/command")).toString();
    }
    if (e.isEmpty())
        e = QStringLiteral("notepad.exe");
    return e;
}

void Commands::openInEditor(const QString& path) {
    const QStringList argv = FsOps::splitCommand(editorCommand());
    if (argv.isEmpty()) {
        minibuffer_->message(QStringLiteral("No editor configured"));
        return;
    }

    QString program = argv.first();
    QStringList args = argv.mid(1);
    bool hasPlaceholder = false;
    for (QString& a : args) {
        if (a.contains(QStringLiteral("%f"))) {
            a.replace(QStringLiteral("%f"), path);
            hasPlaceholder = true;
        }
    }
    if (!hasPlaceholder)
        args.append(path);

    qint64 pid = 0;
    if (QProcess::startDetached(program, args, QFileInfo(path).absolutePath(), &pid))
        minibuffer_->message(QStringLiteral("Opening %1 in %2")
                                 .arg(QFileInfo(path).fileName(), program));
    else
        minibuffer_->message(QStringLiteral("Failed to launch editor: ") + program);
}

void Commands::doFindFile() {
    const DirEntry* e = currentEntry();
    if (!e) {
        minibuffer_->message(QStringLiteral("No file on this line"));
        return;
    }
    const QString path = QDir(model_->currentDir()).filePath(e->name);
    if (e->isDir) {
        const QString canonical = QFileInfo(path).canonicalFilePath();
        loadDirectory(canonical.isEmpty() ? QDir::cleanPath(path) : canonical);
    } else {
        openInEditor(path);
    }
}

void Commands::doUpDirectory() {
    const QString current = model_->currentDir();
    QDir d(current);
    if (!d.cdUp()) {
        minibuffer_->message(QStringLiteral("Already at top level"));
        return;
    }
    const QString cameFrom = QFileInfo(current).fileName();
    loadDirectory(d.absolutePath());
    const int idx = model_->indexOfName(cameFrom);
    buffer_->gotoLine(idx >= 0 ? idx : 0);
}

void Commands::doFindDirectory() {
    minibuffer_->prompt(QStringLiteral("Find directory: "), [this](const QString& text) {
        if (text.isEmpty()) {
            updateEcho();
            return;
        }
        const QString path = expandUser(text);
        const QFileInfo fi(path);
        if (!fi.isDir()) {
            minibuffer_->message(QStringLiteral("Not a directory: ") + text);
            return;
        }
        const QString canonical = fi.canonicalFilePath();
        loadDirectory(canonical.isEmpty() ? fi.absoluteFilePath() : canonical);
    });
}

void Commands::doMark() {
    const DirEntry* e = currentEntry();
    if (!e)
        return;
    const int line = buffer_->currentLine();
    model_->setMarked(e->name, true);
    buffer_->updateMark(line);
    buffer_->moveLine(1);
}

void Commands::doUnmark() {
    const DirEntry* e = currentEntry();
    if (!e)
        return;
    const int line = buffer_->currentLine();
    model_->setMarked(e->name, false);
    model_->setFlagged(e->name, false);
    buffer_->updateMark(line);
    buffer_->moveLine(1);
}

void Commands::doUnmarkBackward() {
    buffer_->moveLine(-1);
    const DirEntry* e = currentEntry();
    if (!e)
        return;
    const int line = buffer_->currentLine();
    model_->setMarked(e->name, false);
    model_->setFlagged(e->name, false);
    buffer_->updateMark(line);
}

void Commands::doUnmarkAll() {
    model_->unmarkAll();
    model_->clearFlags();
    buffer_->refreshText(true);
    minibuffer_->message(QStringLiteral("All marks removed"));
}

void Commands::doToggleMarks() {
    model_->invertMarks();
    buffer_->refreshText(true);
    minibuffer_->message(QStringLiteral("Toggled marks"));
}

void Commands::doFlagDeletion() {
    const DirEntry* e = currentEntry();
    if (!e)
        return;
    if (e->isDotEntry()) {
        minibuffer_->message(QStringLiteral("Refusing to flag \".\" or \"..\""));
        return;
    }
    const int line = buffer_->currentLine();
    model_->setFlagged(e->name, true);
    buffer_->updateMark(line);
    buffer_->moveLine(1);
}

void Commands::doFlaggedDelete() {
    const QStringList names = model_->flaggedNames();
    if (names.isEmpty()) {
        minibuffer_->message(QStringLiteral("No files marked for deletion"));
        return;
    }

    QString preview = names.mid(0, 8).join(QStringLiteral(", "));
    if (names.size() > 8)
        preview += QStringLiteral(", ...");
    const QString question = QStringLiteral("Delete %1 file(s) (%2)?").arg(names.size()).arg(preview);

    minibuffer_->confirm(question, [this, names](bool yes) {
        if (!yes) {
            minibuffer_->message(QStringLiteral("Cancelled"));
            return;
        }
        int ok = 0;
        int failed = 0;
        for (const QString& name : names) {
            const QString path = QDir(model_->currentDir()).filePath(name);
            if (FsOps::removeRecursively(path))
                ++ok;
            else
                ++failed;
        }
        minibuffer_->message(QStringLiteral("Deleted %1 item(s)%2")
                                 .arg(ok)
                                 .arg(failed ? QStringLiteral(", %1 failed").arg(failed)
                                             : QString()));
        reload(false);
    });
}

void Commands::doDeleteImmediate() {
    const DirEntry* e = currentEntry();
    if (!e)
        return;
    if (e->isDotEntry()) {
        minibuffer_->message(QStringLiteral("Refusing to delete \".\" or \"..\""));
        return;
    }
    const QString name = e->name;
    minibuffer_->confirm(QStringLiteral("Delete \"%1\"?").arg(name), [this, name](bool yes) {
        if (!yes) {
            minibuffer_->message(QStringLiteral("Cancelled"));
            return;
        }
        const QString path = QDir(model_->currentDir()).filePath(name);
        if (FsOps::removeRecursively(path))
            minibuffer_->message(QStringLiteral("Deleted %1").arg(name));
        else
            minibuffer_->message(QStringLiteral("Failed to delete %1").arg(name));
        reload(false);
    });
}

void Commands::doCopy() {
    const QStringList names = operationTargets();
    if (names.isEmpty()) {
        minibuffer_->message(QStringLiteral("No files selected"));
        return;
    }
    const QString label = names.size() > 1
                              ? QStringLiteral("Copy %1 files to: ").arg(names.size())
                              : QStringLiteral("Copy %1 to: ").arg(names.first());

    minibuffer_->prompt(label, [this, names](const QString& text) {
        if (text.isEmpty()) {
            updateEcho();
            return;
        }
        const QString resolved = expandUser(text);
        const QFileInfo di(resolved);
        QString targetDir;
        QString singleTarget;
        if (di.isDir())
            targetDir = di.absoluteFilePath();
        else if (names.size() == 1)
            singleTarget = di.absoluteFilePath();
        else {
            minibuffer_->message(QStringLiteral("Target is not a directory: ") + resolved);
            return;
        }

        int ok = 0;
        int failed = 0;
        for (const QString& name : names) {
            const QString src = QDir(model_->currentDir()).filePath(name);
            const QString dst = singleTarget.isEmpty() ? QDir(targetDir).filePath(name) : singleTarget;
            if (QFileInfo(src).absoluteFilePath() == QFileInfo(dst).absoluteFilePath())
                continue;
            if (FsOps::copyRecursively(src, dst))
                ++ok;
            else
                ++failed;
        }
        minibuffer_->message(QStringLiteral("Copied %1 item(s)%2")
                                 .arg(ok)
                                 .arg(failed ? QStringLiteral(", %1 failed").arg(failed)
                                             : QString()));
        reload(false);
    });
}

void Commands::doRename() {
    const QStringList names = operationTargets();
    if (names.isEmpty()) {
        minibuffer_->message(QStringLiteral("No files selected"));
        return;
    }
    const QString label = names.size() > 1
                              ? QStringLiteral("Move %1 files to: ").arg(names.size())
                              : QStringLiteral("Rename %1 to: ").arg(names.first());

    minibuffer_->prompt(label, [this, names](const QString& text) {
        if (text.isEmpty()) {
            updateEcho();
            return;
        }
        const QString resolved = expandUser(text);
        const QFileInfo di(resolved);
        QString targetDir;
        QString singleTarget;
        if (di.isDir())
            targetDir = di.absoluteFilePath();
        else if (names.size() == 1)
            singleTarget = di.absoluteFilePath();
        else {
            minibuffer_->message(QStringLiteral("Target is not a directory: ") + resolved);
            return;
        }

        int ok = 0;
        int failed = 0;
        for (const QString& name : names) {
            const QString src = QDir(model_->currentDir()).filePath(name);
            const QString dst = singleTarget.isEmpty() ? QDir(targetDir).filePath(name) : singleTarget;
            if (QFileInfo(src).absoluteFilePath() == QFileInfo(dst).absoluteFilePath())
                continue;
            if (FsOps::moveTo(src, dst))
                ++ok;
            else
                ++failed;
        }
        minibuffer_->message(QStringLiteral("Moved %1 item(s)%2")
                                 .arg(ok)
                                 .arg(failed ? QStringLiteral(", %1 failed").arg(failed)
                                             : QString()));
        reload(false);
    });
}

void Commands::doCreateDirectory() {
    minibuffer_->prompt(QStringLiteral("Create directory: "), [this](const QString& text) {
        if (text.isEmpty()) {
            updateEcho();
            return;
        }
        QDir d(model_->currentDir());
        if (!d.mkdir(text)) {
            minibuffer_->message(QStringLiteral("Failed to create: ") + text);
            return;
        }
        reload(false);
        const int idx = model_->indexOfName(QFileInfo(text).fileName());
        if (idx >= 0)
            buffer_->gotoLine(idx);
        minibuffer_->message(QStringLiteral("Created %1").arg(text));
    });
}

void Commands::doSortToggle() {
    switch (model_->sortMode()) {
    case DirModel::SortMode::Name:
        model_->setSortMode(DirModel::SortMode::Size);
        break;
    case DirModel::SortMode::Size:
        model_->setSortMode(DirModel::SortMode::Time);
        break;
    case DirModel::SortMode::Time:
        model_->setSortMode(DirModel::SortMode::Name);
        break;
    }
    reload(true);
    minibuffer_->message(QStringLiteral("Sorting by %1").arg(model_->sortModeName()));
}

void Commands::doHelp() {
    static const char* help = R"(Dired-like key bindings

Navigation
  n / C-n / <down>   next line          p / C-p / <up>   previous line
  <left> / C-b       backward char      <right> / C-f    forward char
  C-a / <home>       beginning of line  C-e / <end>      end of line
  M-<                beginning of buffer  M->            end of buffer
  C-v / <next>       scroll down        M-v / <prior>    scroll up
  C-l                recenter

Files
  RET / f            open file (external editor) or enter directory
  ^                  go to parent directory
  C-x C-f            find directory (prompt)

Marks
  m                  mark                 u / DEL / C-d    unmark
  U                  unmark all           t                toggle marks

Deletion
  d                  flag for deletion    x                execute deletions
  D                  delete this file now

Operations
  C                  copy                 R                rename / move
  +                  create directory     g                revert (refresh)
  s                  change sort order

Other
  ?                  this help            q / C-x C-c      quit
  C-g                cancel

Editor: set the EDITOR environment variable or the QSettings key editor/command.)";
    QMessageBox::information(window_, QStringLiteral("Dired keys"), QString::fromLatin1(help));
}
