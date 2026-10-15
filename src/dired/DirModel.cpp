#include "dired/DirModel.h"

#include "util/Format.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace dired {

DirEntry DirModel::makeEntry(const QFileInfo& fi, const QString& name, int dotRank) const {
    DirEntry e;
    e.name = name;
    e.dotRank = dotRank;
    e.isSymlink = dotRank < 2 ? false : fi.isSymLink();
    e.isDir = fi.isDir();
    e.isExecutable = !e.isDir && fi.isExecutable();
    e.size = e.isDir ? 0 : fi.size();
    e.lastModified = fi.lastModified();
    e.permissions = Format::permissions(fi, e.isSymlink, e.isDir);
    e.sizeStr = QString::number(e.size);
    e.dateStr = Format::dateTime(fi.lastModified());
    e.ownerDisplay = Format::owner(fi);
    if (e.isSymlink)
        e.symlinkTarget = fi.symLinkTarget();

    e.displayName = name;
    if (!e.symlinkTarget.isEmpty())
        e.displayName += QStringLiteral(" -> ") + e.symlinkTarget;
    else if (e.isDir && dotRank >= 2)
        e.displayName += QLatin1Char('/');
    return e;
}

bool DirModel::load(const QString& dir, QString* error) {
    QDir d(dir);
    if (!d.exists()) {
        if (error)
            *error = QStringLiteral("Directory does not exist: ") + dir;
        return false;
    }
    currentDir_ = d.absolutePath();

    const QFileInfoList list =
        d.entryInfoList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot,
                        QDir::NoSort);

    QList<DirEntry> newEntries;
    newEntries.reserve(list.size() + 2);
    newEntries.append(makeEntry(QFileInfo(currentDir_), QStringLiteral("."), 0));
    newEntries.append(makeEntry(QFileInfo(d.filePath(QStringLiteral(".."))), QStringLiteral(".."), 1));
    for (const QFileInfo& fi : list)
        newEntries.append(makeEntry(fi, fi.fileName(), 2));

    sortEntries(newEntries);
    entries_ = std::move(newEntries);

    // Drop marks / flags for files that no longer exist.
    QSet<QString> present;
    present.reserve(entries_.size());
    for (const DirEntry& e : entries_)
        present.insert(e.name);
    for (auto it = marked_.begin(); it != marked_.end();) {
        if (present.contains(*it))
            ++it;
        else
            it = marked_.erase(it);
    }
    for (auto it = flagged_.begin(); it != flagged_.end();) {
        if (present.contains(*it))
            ++it;
        else
            it = flagged_.erase(it);
    }
    return true;
}

void DirModel::sortEntries(QList<DirEntry>& list) const {
    const SortMode mode = sort_;
    std::stable_sort(list.begin(), list.end(),
                     [mode](const DirEntry& a, const DirEntry& b) {
                         if (a.dotRank != b.dotRank)
                             return a.dotRank < b.dotRank;
                         if (a.dotRank < 2)
                             return false;
                         if (mode == SortMode::Size && a.size != b.size)
                             return a.size > b.size;
                         if (mode == SortMode::Time && a.lastModified != b.lastModified)
                             return a.lastModified > b.lastModified;
                         return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                     });
}

int DirModel::indexOfName(const QString& name) const {
    for (int i = 0; i < entries_.size(); ++i) {
        if (entries_.at(i).name == name)
            return i;
    }
    return -1;
}

QChar DirModel::markChar(int i) const {
    const DirEntry& e = entries_.at(i);
    if (flagged_.contains(e.name))
        return QChar('D');
    if (marked_.contains(e.name))
        return QChar('*');
    return QChar(' ');
}

QString DirModel::lineText(int i) const {
    const DirEntry& e = entries_.at(i);
    QString line;
    line += markChar(i);
    line += QLatin1Char(' ');
    line += e.permissions;
    line += QLatin1Char(' ');
    line += e.sizeStr.rightJustified(10, QLatin1Char(' '));
    line += QLatin1Char(' ');
    line += e.dateStr.leftJustified(12, QLatin1Char(' '));
    line += QLatin1Char(' ');
    line += e.ownerDisplay.leftJustified(12, QLatin1Char(' '));
    line += QLatin1Char(' ');
    line += e.displayName;
    return line;
}

QString DirModel::plainText() const {
    QStringList lines;
    lines.reserve(entries_.size());
    for (int i = 0; i < entries_.size(); ++i)
        lines << lineText(i);
    return lines.join(QLatin1Char('\n'));
}

void DirModel::setMarked(const QString& name, bool on) {
    if (on) {
        marked_.insert(name);
        flagged_.remove(name);
    } else {
        marked_.remove(name);
    }
}

void DirModel::setFlagged(const QString& name, bool on) {
    if (on) {
        flagged_.insert(name);
        marked_.remove(name);
    } else {
        flagged_.remove(name);
    }
}

void DirModel::unmarkAll() { marked_.clear(); }

void DirModel::clearFlags() { flagged_.clear(); }

void DirModel::invertMarks() {
    QSet<QString> inverted;
    for (int i = 2; i < entries_.size(); ++i) {
        const QString& name = entries_.at(i).name;
        if (!marked_.contains(name))
            inverted.insert(name);
    }
    marked_ = inverted;
}

QStringList DirModel::markedNames() const {
    QStringList l(marked_.begin(), marked_.end());
    l.sort();
    return l;
}

QStringList DirModel::flaggedNames() const {
    QStringList l(flagged_.begin(), flagged_.end());
    l.sort();
    return l;
}

QString DirModel::sortModeName() const {
    switch (sort_) {
    case SortMode::Name:
        return QStringLiteral("name");
    case SortMode::Size:
        return QStringLiteral("size");
    case SortMode::Time:
        return QStringLiteral("time");
    }
    return QStringLiteral("name");
}

} // namespace dired
