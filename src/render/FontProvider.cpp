#include "FontProvider.h"

#include <QByteArray>
#include <QFileInfo>
#include <QFontDatabase>
#include <QString>
#include <QStringList>

namespace {

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

    // Bundled into the executable at build time (see CMakeLists.txt).
    // Either format may be present depending on which mirror was used.
    static const QStringList kResourceCandidates = {
        QStringLiteral(":/fonts/NotoSansJP-Regular.otf"),
        QStringLiteral(":/fonts/NotoSansJP-Regular.ttf"),
    };
    for (const QString& path : kResourceCandidates) {
        if (QFileInfo::exists(path)) {
            const QString family = registerAndPickFamily(path);
            if (!family.isEmpty()) return family;
        }
    }

    return pickInstalledFamily();
}

QString FontProvider::installHint() {
    return QStringLiteral(
        "CJK font not found. Install one of:\n"
        "  Arch:   sudo pacman -S noto-fonts-cjk\n"
        "  Debian: sudo apt install fonts-noto-cjk\n"
        "  Fedora: sudo dnf install google-noto-sans-cjk-fonts\n"
        "Or place NotoSansJP-Regular.otf at resources/fonts/ and rebuild,\n"
        "or set FLASHCARDS_FONT_PATH to a TTF/OTF file."
    );
}
