#include "app/MainWindow.h"

#include "dired/Commands.h"
#include "dired/DiredBuffer.h"
#include "dired/KeyMap.h"
#include "editor/EditorBuffer.h"
#include "editor/EditorCommands.h"
#include "ui/Minibuffer.h"

#include <QDir>
#include <QFileInfo>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(const QString& startDir, QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stack_ = new QStackedWidget(central);
    dired_ = new DiredBuffer(stack_);
    editor_ = new EditorBuffer(stack_);
    stack_->addWidget(dired_);   // index 0
    stack_->addWidget(editor_);  // index 1
    stack_->setCurrentWidget(dired_);

    minibuffer_ = new Minibuffer(central);
    layout->addWidget(stack_, 1);
    layout->addWidget(minibuffer_);
    setCentralWidget(central);

    dired_->setModel(&model_);

    commands_ = new Commands(&model_, dired_, minibuffer_, this, this);
    commands_->setEditFileCallback([this](const QString& path) { openInEditor(path); });

    editorCommands_ = new EditorCommands(editor_, minibuffer_, this, this);
    editorCommands_->setCloseCallback([this] { closeEditor(); });

    diredKeys_ = new KeyMap(this);
    setupDiredKeyMap();
    dired_->installEventFilter(diredKeys_);

    editorKeys_ = new KeyMap(this);
    editorKeys_->setPassThroughUndefined(true); // let unbound keys insert text
    setupEditorKeyMap();
    editor_->installEventFilter(editorKeys_);

    connect(diredKeys_, &KeyMap::commandTriggered, commands_, &Commands::run);
    connect(diredKeys_, &KeyMap::prefixChanged, this, &MainWindow::onPrefixChanged);
    connect(diredKeys_, &KeyMap::undefinedKey, this, &MainWindow::onUndefinedKey);

    connect(editorKeys_, &KeyMap::commandTriggered, editorCommands_, &EditorCommands::run);
    connect(editorKeys_, &KeyMap::prefixChanged, this, &MainWindow::onEditorPrefixChanged);
    connect(editorKeys_, &KeyMap::undefinedKey, this, &MainWindow::onEditorUndefinedKey);

    connect(minibuffer_, &Minibuffer::finished, this, [this] { focusActive(); });

    QString dir = startDir;
    const QFileInfo fi(dir);
    if (dir.isEmpty() || !fi.exists())
        dir = QDir::currentPath();
    else if (fi.isFile())
        dir = fi.absolutePath();

    commands_->loadDirectory(dir);
    dired_->setFocus();
    QTimer::singleShot(0, dired_, [this] { dired_->setFocus(Qt::OtherFocusReason); });
}

void MainWindow::setupDiredKeyMap() {
    diredKeys_->bind(QStringLiteral("n"), QStringLiteral("next-line"));
    diredKeys_->bind(QStringLiteral("C-n"), QStringLiteral("next-line"));
    diredKeys_->bind(QStringLiteral("<down>"), QStringLiteral("next-line"));
    diredKeys_->bind(QStringLiteral("p"), QStringLiteral("previous-line"));
    diredKeys_->bind(QStringLiteral("C-p"), QStringLiteral("previous-line"));
    diredKeys_->bind(QStringLiteral("<up>"), QStringLiteral("previous-line"));
    diredKeys_->bind(QStringLiteral("C-f"), QStringLiteral("forward-char"));
    diredKeys_->bind(QStringLiteral("<right>"), QStringLiteral("forward-char"));
    diredKeys_->bind(QStringLiteral("C-b"), QStringLiteral("backward-char"));
    diredKeys_->bind(QStringLiteral("<left>"), QStringLiteral("backward-char"));
    diredKeys_->bind(QStringLiteral("C-a"), QStringLiteral("move-beginning-of-line"));
    diredKeys_->bind(QStringLiteral("<home>"), QStringLiteral("move-beginning-of-line"));
    diredKeys_->bind(QStringLiteral("C-e"), QStringLiteral("move-end-of-line"));
    diredKeys_->bind(QStringLiteral("<end>"), QStringLiteral("move-end-of-line"));
    diredKeys_->bind(QStringLiteral("M-<"), QStringLiteral("beginning-of-buffer"));
    diredKeys_->bind(QStringLiteral("M->"), QStringLiteral("end-of-buffer"));
    diredKeys_->bind(QStringLiteral("C-v"), QStringLiteral("scroll-up"));
    diredKeys_->bind(QStringLiteral("<next>"), QStringLiteral("scroll-up"));
    diredKeys_->bind(QStringLiteral("M-v"), QStringLiteral("scroll-down"));
    diredKeys_->bind(QStringLiteral("<prior>"), QStringLiteral("scroll-down"));
    diredKeys_->bind(QStringLiteral("C-l"), QStringLiteral("recenter"));

    diredKeys_->bind(QStringLiteral("RET"), QStringLiteral("dired-find-file"));
    diredKeys_->bind(QStringLiteral("f"), QStringLiteral("dired-find-file"));
    diredKeys_->bind(QStringLiteral("^"), QStringLiteral("dired-up-directory"));
    diredKeys_->bind(QStringLiteral("C-x C-f"), QStringLiteral("dired-find-directory"));

    diredKeys_->bind(QStringLiteral("m"), QStringLiteral("dired-mark"));
    diredKeys_->bind(QStringLiteral("u"), QStringLiteral("dired-unmark"));
    diredKeys_->bind(QStringLiteral("DEL"), QStringLiteral("dired-unmark-backward"));
    diredKeys_->bind(QStringLiteral("C-d"), QStringLiteral("dired-unmark-backward"));
    diredKeys_->bind(QStringLiteral("U"), QStringLiteral("dired-unmark-all-marks"));
    diredKeys_->bind(QStringLiteral("t"), QStringLiteral("dired-toggle-marks"));

    diredKeys_->bind(QStringLiteral("d"), QStringLiteral("dired-flag-file-deletion"));
    diredKeys_->bind(QStringLiteral("x"), QStringLiteral("dired-do-flagged-delete"));
    diredKeys_->bind(QStringLiteral("D"), QStringLiteral("dired-do-delete"));
    diredKeys_->bind(QStringLiteral("C"), QStringLiteral("dired-do-copy"));
    diredKeys_->bind(QStringLiteral("R"), QStringLiteral("dired-do-rename"));
    diredKeys_->bind(QStringLiteral("+"), QStringLiteral("dired-create-directory"));

    diredKeys_->bind(QStringLiteral("g"), QStringLiteral("revert-buffer"));
    diredKeys_->bind(QStringLiteral("s"), QStringLiteral("dired-sort-toggle"));
    diredKeys_->bind(QStringLiteral("?"), QStringLiteral("dired-help"));
    diredKeys_->bind(QStringLiteral("C-g"), QStringLiteral("keyboard-quit"));
    diredKeys_->bind(QStringLiteral("q"), QStringLiteral("quit-window"));
    diredKeys_->bind(QStringLiteral("C-x C-c"), QStringLiteral("save-buffers-kill-emacs"));
}

void MainWindow::setupEditorKeyMap() {
    editorKeys_->bind(QStringLiteral("C-f"), QStringLiteral("forward-char"));
    editorKeys_->bind(QStringLiteral("<right>"), QStringLiteral("forward-char"));
    editorKeys_->bind(QStringLiteral("C-b"), QStringLiteral("backward-char"));
    editorKeys_->bind(QStringLiteral("<left>"), QStringLiteral("backward-char"));
    editorKeys_->bind(QStringLiteral("C-n"), QStringLiteral("next-line"));
    editorKeys_->bind(QStringLiteral("<down>"), QStringLiteral("next-line"));
    editorKeys_->bind(QStringLiteral("C-p"), QStringLiteral("previous-line"));
    editorKeys_->bind(QStringLiteral("<up>"), QStringLiteral("previous-line"));
    editorKeys_->bind(QStringLiteral("C-a"), QStringLiteral("move-beginning-of-line"));
    editorKeys_->bind(QStringLiteral("<home>"), QStringLiteral("move-beginning-of-line"));
    editorKeys_->bind(QStringLiteral("C-e"), QStringLiteral("move-end-of-line"));
    editorKeys_->bind(QStringLiteral("<end>"), QStringLiteral("move-end-of-line"));
    editorKeys_->bind(QStringLiteral("M-<"), QStringLiteral("beginning-of-buffer"));
    editorKeys_->bind(QStringLiteral("M->"), QStringLiteral("end-of-buffer"));
    editorKeys_->bind(QStringLiteral("C-v"), QStringLiteral("scroll-up"));
    editorKeys_->bind(QStringLiteral("<next>"), QStringLiteral("scroll-up"));
    editorKeys_->bind(QStringLiteral("M-v"), QStringLiteral("scroll-down"));
    editorKeys_->bind(QStringLiteral("<prior>"), QStringLiteral("scroll-down"));
    editorKeys_->bind(QStringLiteral("C-l"), QStringLiteral("recenter"));

    editorKeys_->bind(QStringLiteral("RET"), QStringLiteral("newline"));
    editorKeys_->bind(QStringLiteral("DEL"), QStringLiteral("delete-backward-char"));
    editorKeys_->bind(QStringLiteral("C-d"), QStringLiteral("delete-char"));
    editorKeys_->bind(QStringLiteral("C-o"), QStringLiteral("open-line"));

    editorKeys_->bind(QStringLiteral("C-SPC"), QStringLiteral("set-mark-command"));
    editorKeys_->bind(QStringLiteral("C-g"), QStringLiteral("keyboard-quit"));
    editorKeys_->bind(QStringLiteral("C-k"), QStringLiteral("kill-line"));
    editorKeys_->bind(QStringLiteral("C-w"), QStringLiteral("kill-region"));
    editorKeys_->bind(QStringLiteral("M-w"), QStringLiteral("kill-ring-save"));
    editorKeys_->bind(QStringLiteral("C-y"), QStringLiteral("yank"));

    editorKeys_->bind(QStringLiteral("C-/"), QStringLiteral("undo"));
    editorKeys_->bind(QStringLiteral("C-x u"), QStringLiteral("undo"));
    editorKeys_->bind(QStringLiteral("C-s"), QStringLiteral("search-forward"));

    editorKeys_->bind(QStringLiteral("C-x C-s"), QStringLiteral("save-buffer"));
    editorKeys_->bind(QStringLiteral("C-x C-w"), QStringLiteral("write-file"));
    editorKeys_->bind(QStringLiteral("C-x k"), QStringLiteral("kill-buffer"));
    editorKeys_->bind(QStringLiteral("C-x C-c"), QStringLiteral("save-buffers-kill-emacs"));
}

void MainWindow::openInEditor(const QString& path) {
    QString error;
    if (!editor_->loadFile(path, &error)) {
        minibuffer_->message(QStringLiteral("Cannot open %1: %2")
                                 .arg(QFileInfo(path).fileName(), error));
        return;
    }
    stack_->setCurrentWidget(editor_);
    setWindowTitle(QStringLiteral("emacs: ") + QDir::toNativeSeparators(editor_->filePath()));
    editor_->setFocus(Qt::OtherFocusReason);
    editorCommands_->refreshStatus();
    minibuffer_->message(QStringLiteral("Opened %1  [C-x C-s save, C-x k close]")
                             .arg(editor_->fileName()));
}

void MainWindow::closeEditor() {
    stack_->setCurrentWidget(dired_);
    commands_->refresh();
    focusActive();
}

void MainWindow::focusActive() {
    if (editorActive())
        editor_->setFocus(Qt::OtherFocusReason);
    else
        dired_->setFocus(Qt::OtherFocusReason);
}

bool MainWindow::editorActive() const { return stack_->currentWidget() == editor_; }

void MainWindow::onPrefixChanged(const QString& prefix) {
    if (prefix.isEmpty())
        return; // the triggering command / reload will refresh the echo area
    minibuffer_->message(prefix + QStringLiteral("-"));
}

void MainWindow::onUndefinedKey(const QString& key) {
    minibuffer_->message(QStringLiteral("%1 is undefined").arg(key));
}

void MainWindow::onEditorPrefixChanged(const QString& prefix) {
    if (prefix.isEmpty())
        return;
    minibuffer_->message(prefix + QStringLiteral("-"));
}

void MainWindow::onEditorUndefinedKey(const QString& key) {
    minibuffer_->message(QStringLiteral("%1 is undefined").arg(key));
}
