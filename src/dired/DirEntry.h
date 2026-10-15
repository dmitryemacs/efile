#pragma once

#include <QDateTime>
#include <QString>

namespace dired {

// One directory entry, pre-formatted for display in the listing buffer.
struct DirEntry {
    QString name;         // bare file name (including "." and "..")
    QString displayName;  // name with "/" for directories or " -> target" for symlinks
    QString permissions;  // ls-style permission string, 10 chars
    QString sizeStr;      // pre-formatted size in bytes
    QString dateStr;      // pre-formatted modification date
    QString ownerDisplay; // owner name or "-"

    qint64 size = 0;
    QDateTime lastModified;
    QString symlinkTarget;

    bool isDir = false;
    bool isSymlink = false;
    bool isExecutable = false;

    // 0 for ".", 1 for "..", 2 for a regular entry. Keeps dot entries first.
    int dotRank = 2;

    bool isDotEntry() const { return dotRank < 2; }
};

} // namespace dired
