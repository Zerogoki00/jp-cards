#include "FontProvider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

namespace {

// Owns the on-disk copies of extracted resource fonts so the files stay valid
// for QFontDatabase / Qt PDF for the lifetime of the process. QTemporaryFile
// auto-removes its backing file on destruction; making the list static lives
// until program exit.
QList<QTemporaryFile*>& tempFontHolder() {
    static QList<QTemporaryFile*> store;
    return store;
}

// Extract a Qt resource font onto a real on-disk path. Why we bother:
//
//   addApplicationFont(":/...") internally reads the bytes and registers the
//   font via addApplicationFontFromData. On Qt 6.8.3 / Windows the QPdfWriter
//   font-embedding code does NOT properly embed fonts registered that way —
//   it falls back to converting glyphs to vector outline paths, which prints
//   shifted on Windows printers (different driver subsystem, asymmetric
//   physical margins). Loading from a real on-disk file path makes Qt embed
//   the font as a CID TrueType subset, just like on Linux.
//
// Returns the on-disk path, or an empty string on failure.
QString extractResourceToDisk(const QString& resourcePath) {
    QFile src(resourcePath);
    if (!src.open(QIODevice::ReadOnly)) return {};
    const QByteArray data = src.readAll();
    if (data.isEmpty()) return {};

    const QString suffix = QFileInfo(resourcePath).suffix();
    auto* tmp = new QTemporaryFile(
        QDir::tempPath() + QStringLiteral("/jp-cards-font-XXXXXX.") + suffix);
    if (!tmp->open()) { delete tmp; return {}; }
    if (tmp->write(data) != data.size()) { delete tmp; return {}; }
    tmp->flush();
    const QString path = tmp->fileName();
    tempFontHolder().append(tmp);  // keep alive for program lifetime
    return path;
}

QString registerAndPickFamily(const QString& path) {
    const int id = QFontDatabase::addApplicationFont(path);
    if (id < 0) return {};
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) return {};
    for (const QString& family : families) {
        if (family == QStringLiteral("Noto Sans JP")) return family;
    }
    for (const QString& family : families) {
        if (family.contains(QStringLiteral("Regular"))) return family;
    }
    return families.first();
}

} // namespace

QString FontProvider::resolveFamily() {
    // Bundled into the executable at build time (see CMakeLists.txt). Use TTF:
    // glyf-based fonts embed reliably in Qt PDF on every platform, while
    // CFF/OTF embedding is unreliable on Qt-Windows builds.
    static const QStringList kResourceCandidates = {
        QStringLiteral(":/fonts/NotoSansJP-Regular.ttf"),
    };
    for (const QString& resPath : kResourceCandidates) {
        if (!QFileInfo::exists(resPath)) continue;
        const QString diskPath = extractResourceToDisk(resPath);
        if (diskPath.isEmpty()) continue;
        const QString family = registerAndPickFamily(diskPath);
        if (!family.isEmpty()) return family;
    }

    return {};
}

QString FontProvider::installHint() {
    return QStringLiteral(
        "Bundled CJK font not found. Place NotoSansJP-Regular.ttf at "
        "resources/fonts/ and rebuild."
    );
}
