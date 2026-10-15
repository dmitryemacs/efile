#include "app/MainWindow.h"

#include "dired/Commands.h"
#include "dired/DiredBuffer.h"
#include "dired/KeyMap.h"
#include "ui/Minibuffer.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(const QString& startDir, QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    buffer_ = new DiredBuffer(central);
    minibuffer_ = new Minibuffer(central);
    layout->addWidget(buffer_, 1);
    layout->addWidget(minibuffer_);
    setCentralWidget(central);

    buffer_->setModel(&model_);
    commands_ = new Commands(&model_, buffer_, minibuffer_, this, this);

    keymap_ = new KeyMap(this);
    setupKeyMap();
    buffer_->installEventFilter(keymap_);

    connect(keymap_, &KeyMap::commandTriggered, commands_, &Commands::run);
    connect(keymap_, &KeyMap::prefixChanged, this, &MainWindow::onPrefixChanged);
    connect(keymap_, &KeyMap::undefinedKey, this, &MainWindow::onUndefinedKey);
    connect(minibuffer_, &Minibuffer::finished, this, [this] { buffer_->setFocus(); });

    QString dir = startDir;
    const QFileInfo fi(dir);
    if (dir.isEmpty() || !fi.exists())
        dir = QDir::currentPath();
    else if (fi.isFile())
        dir = fi.absolutePath();

    commands_->loadDirectory(dir);
    buffer_->setFocus();
    QTimer::singleShot(0, buffer_, [this] { buffer_->setFocus(Qt::OtherFocusReason); });
}

void MainWindow::setupKeyMap() {
    keymap_->bind(QStringLiteral("n"), QStringLiteral("next-line"));
    keymap_->bind(QStringLiteral("C-n"), QStringLiteral("next-line"));
    keymap_->bind(QStringLiteral("<down>"), QStringLiteral("next-line"));
    keymap_->bind(QStringLiteral("p"), QStringLiteral("previous-line"));
    keymap_->bind(QStringLiteral("C-p"), QStringLiteral("previous-line"));
    keymap_->bind(QStringLiteral("<up>"), QStringLiteral("previous-line"));
    keymap_->bind(QStringLiteral("C-f"), QStringLiteral("forward-char"));
    keymap_->bind(QStringLiteral("<right>"), QStringLiteral("forward-char"));
    keymap_->bind(QStringLiteral("C-b"), QStringLiteral("backward-char"));
    keymap_->bind(QStringLiteral("<left>"), QStringLiteral("backward-char"));
    keymap_->bind(QStringLiteral("C-a"), QStringLiteral("move-beginning-of-line"));
    keymap_->bind(QStringLiteral("<home>"), QStringLiteral("move-beginning-of-line"));
    keymap_->bind(QStringLiteral("C-e"), QStringLiteral("move-end-of-line"));
    keymap_->bind(QStringLiteral("<end>"), QStringLiteral("move-end-of-line"));
    keymap_->bind(QStringLiteral("M-<"), QStringLiteral("beginning-of-buffer"));
    keymap_->bind(QStringLiteral("M->"), QStringLiteral("end-of-buffer"));
    keymap_->bind(QStringLiteral("C-v"), QStringLiteral("scroll-up"));
    keymap_->bind(QStringLiteral("<next>"), QStringLiteral("scroll-up"));
    keymap_->bind(QStringLiteral("M-v"), QStringLiteral("scroll-down"));
    keymap_->bind(QStringLiteral("<prior>"), QStringLiteral("scroll-down"));
    keymap_->bind(QStringLiteral("C-l"), QStringLiteral("recenter"));

    keymap_->bind(QStringLiteral("RET"), QStringLiteral("dired-find-file"));
    keymap_->bind(QStringLiteral("f"), QStringLiteral("dired-find-file"));
    keymap_->bind(QStringLiteral("^"), QStringLiteral("dired-up-directory"));
    keymap_->bind(QStringLiteral("C-x C-f"), QStringLiteral("dired-find-directory"));

    keymap_->bind(QStringLiteral("m"), QStringLiteral("dired-mark"));
    keymap_->bind(QStringLiteral("u"), QStringLiteral("dired-unmark"));
    keymap_->bind(QStringLiteral("DEL"), QStringLiteral("dired-unmark-backward"));
    keymap_->bind(QStringLiteral("C-d"), QStringLiteral("dired-unmark-backward"));
    keymap_->bind(QStringLiteral("U"), QStringLiteral("dired-unmark-all-marks"));
    keymap_->bind(QStringLiteral("t"), QStringLiteral("dired-toggle-marks"));

    keymap_->bind(QStringLiteral("d"), QStringLiteral("dired-flag-file-deletion"));
    keymap_->bind(QStringLiteral("x"), QStringLiteral("dired-do-flagged-delete"));
    keymap_->bind(QStringLiteral("D"), QStringLiteral("dired-do-delete"));
    keymap_->bind(QStringLiteral("C"), QStringLiteral("dired-do-copy"));
    keymap_->bind(QStringLiteral("R"), QStringLiteral("dired-do-rename"));
    keymap_->bind(QStringLiteral("+"), QStringLiteral("dired-create-directory"));

    keymap_->bind(QStringLiteral("g"), QStringLiteral("revert-buffer"));
    keymap_->bind(QStringLiteral("s"), QStringLiteral("dired-sort-toggle"));
    keymap_->bind(QStringLiteral("?"), QStringLiteral("dired-help"));
    keymap_->bind(QStringLiteral("C-g"), QStringLiteral("keyboard-quit"));
    keymap_->bind(QStringLiteral("q"), QStringLiteral("quit-window"));
    keymap_->bind(QStringLiteral("C-x C-c"), QStringLiteral("save-buffers-kill-emacs"));
}

void MainWindow::onPrefixChanged(const QString& prefix) {
    if (prefix.isEmpty())
        return; // the triggering command / reload will refresh the echo area
    minibuffer_->message(prefix + QStringLiteral("-"));
}

void MainWindow::onUndefinedKey(const QString& key) {
    minibuffer_->message(QStringLiteral("%1 is undefined").arg(key));
}
