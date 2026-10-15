#pragma once

#include "dired/DirModel.h"

#include <QMainWindow>

class DiredBuffer;
class EditorBuffer;
class Minibuffer;
class Commands;
class EditorCommands;
class KeyMap;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& startDir, QWidget* parent = nullptr);

private slots:
    void onPrefixChanged(const QString& prefix);
    void onUndefinedKey(const QString& key);
    void onEditorPrefixChanged(const QString& prefix);
    void onEditorUndefinedKey(const QString& key);

private:
    void setupDiredKeyMap();
    void setupEditorKeyMap();

    void openInEditor(const QString& path);
    void closeEditor();
    void focusActive();
    bool editorActive() const;

    dired::DirModel model_;
    DiredBuffer* dired_ = nullptr;
    EditorBuffer* editor_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    Minibuffer* minibuffer_ = nullptr;
    Commands* commands_ = nullptr;
    EditorCommands* editorCommands_ = nullptr;
    KeyMap* diredKeys_ = nullptr;
    KeyMap* editorKeys_ = nullptr;
};
