#pragma once

#include "dired/DirEntry.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

class QFileInfo;

namespace dired {

// Reads a directory and exposes dired-style listing lines, marks and deletion flags.
class DirModel {
public:
    enum class SortMode { Name, Size, Time };

    bool load(const QString& dir, QString* error = nullptr);

    const QString& currentDir() const { return currentDir_; }
    int count() const { return static_cast<int>(entries_.size()); }
    const DirEntry& at(int i) const { return entries_.at(i); }
    int indexOfName(const QString& name) const;

    QChar markChar(int i) const;
    QString lineText(int i) const;
    QString plainText() const;

    bool isMarked(const QString& name) const { return marked_.contains(name); }
    bool isFlagged(const QString& name) const { return flagged_.contains(name); }
    void setMarked(const QString& name, bool on);
    void setFlagged(const QString& name, bool on);
    void unmarkAll();
    void clearFlags();
    void invertMarks();
    QStringList markedNames() const;
    QStringList flaggedNames() const;
    int markedCount() const { return static_cast<int>(marked_.size()); }
    int flaggedCount() const { return static_cast<int>(flagged_.size()); }

    SortMode sortMode() const { return sort_; }
    void setSortMode(SortMode mode) { sort_ = mode; }
    QString sortModeName() const;

private:
    DirEntry makeEntry(const QFileInfo& fi, const QString& name, int dotRank) const;
    void sortEntries(QList<DirEntry>& list) const;

    QString currentDir_;
    QList<DirEntry> entries_;
    QSet<QString> marked_;
    QSet<QString> flagged_;
    SortMode sort_ = SortMode::Name;
};

} // namespace dired
