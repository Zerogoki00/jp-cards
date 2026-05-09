#include "FontProvider.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QList>
#include <QStandardPaths>
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
    return families.isEmpty() ? QString() : families.first();
}

QString pickInstalledFamily() {
    static const QStringList kCandidates = {
        QStringLiteral("Noto Sans CJK JP"),
        QStringLiteral("Noto Sans JP"),
        QStringLiteral("Source Han Sans JP"),
        QStringLiteral("Source Han Sans"),
        QStringLiteral("Yu Gothic"),
        QStringLiteral("Meiryo"),
        QStringLiteral("MS Gothic"),
        QStringLiteral("IPAGothic"),
        QStringLiteral("TakaoGothic"),
        QStringLiteral("Sazanami Gothic"),
    };
    const QStringList installed = QFontDatabase::families();
    for (const QString& name : kCandidates) {
        if (installed.contains(name)) return name;
    }
    return {};
}

} // namespace

QString FontProvider::resolveFamily() {
    const QByteArray envPath = qgetenv("FLASHCARDS_FONT_PATH");
    if (!envPath.isEmpty()) {
        const QString p = QString::fromLocal8Bit(envPath);
        if (QFileInfo::exists(p)) {
            const QString family = registerAndPickFamily(p);
            if (!family.isEmpty()) return family;
        }
    }

    // Bundled into the executable at build time (see CMakeLists.txt). Either
    // format may be present depending on which mirror was used to download it.
    // TTF is tried first: glyf-based fonts embed reliably in Qt PDF on every
    // platform; CFF/OTF embedding is unreliable on Qt-Windows builds.
    static const QStringList kResourceCandidates = {
        QStringLiteral(":/fonts/NotoSansJP-Regular.ttf"),
        QStringLiteral(":/fonts/NotoSansJP-Regular.otf"),
    };
    for (const QString& resPath : kResourceCandidates) {
        if (!QFileInfo::exists(resPath)) continue;
        const QString diskPath = extractResourceToDisk(resPath);
        if (diskPath.isEmpty()) continue;
        const QString family = registerAndPickFamily(diskPath);
        if (!family.isEmpty()) return family;
    }

    return pickInstalledFamily();
}

QString FontProvider::installHint() {
    return QStringLiteral(
        "CJK font not found. Install one of:\n"
        "  Arch:   sudo pacman -S noto-fonts-cjk\n"
        "  Debian: sudo apt install fonts-noto-cjk\n"
        "  Fedora: sudo dnf install google-noto-sans-cjk-fonts\n"
        "Or place NotoSansJP-Regular.ttf at resources/fonts/ and rebuild,\n"
        "or set FLASHCARDS_FONT_PATH to a TTF/OTF file."
    );
}
