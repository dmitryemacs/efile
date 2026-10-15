#pragma once

#include "dired/DirModel.h"

#include <QMainWindow>

class DiredBuffer;
class Minibuffer;
class Commands;
class KeyMap;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& startDir, QWidget* parent = nullptr);

private slots:
    void onPrefixChanged(const QString& prefix);
    void onUndefinedKey(const QString& key);

private:
    void setupKeyMap();

    dired::DirModel model_;
    DiredBuffer* buffer_ = nullptr;
    Minibuffer* minibuffer_ = nullptr;
    Commands* commands_ = nullptr;
    KeyMap* keymap_ = nullptr;
};
