#include "deviceName.h"

#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QSysInfo>

#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_utils.h"
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    #include <SystemConfiguration/SystemConfiguration.h>
#endif

namespace
{
bool isUsable(const QString &name)
{
    const QString lower = name.trimmed().toLower();
    return !lower.isEmpty() && lower != QLatin1String("localhost") && !lower.startsWith(QLatin1String("localhost."));
}

#if defined(Q_OS_ANDROID)
QString androidName()
{
    // "Device name" from Android settings (API 25+), what the user sees in Settings > About phone
    const QJniObject context = AndroidUtils::getActivity();
    if (context.isValid()) {
        const QJniObject resolver = context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        if (resolver.isValid()) {
            const QJniObject key = QJniObject::fromString(QStringLiteral("device_name"));
            const QJniObject value = QJniObject::callStaticObjectMethod(
                    "android/provider/Settings$Global", "getString",
                    "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", resolver.object(),
                    key.object<jstring>());
            // QJniObject clears a Java exception itself and returns an invalid object
            if (value.isValid() && isUsable(value.toString())) {
                return value.toString().trimmed();
            }
        }
    }

    const QString manufacturer =
            QJniObject::getStaticObjectField<jstring>("android/os/Build", "MANUFACTURER").toString().trimmed();
    const QString model = QJniObject::getStaticObjectField<jstring>("android/os/Build", "MODEL").toString().trimmed();
    if (model.isEmpty()) {
        return manufacturer;
    }
    if (manufacturer.isEmpty() || model.startsWith(manufacturer, Qt::CaseInsensitive)) {
        return model;
    }
    return manufacturer.left(1).toUpper() + manufacturer.mid(1) + QLatin1Char(' ') + model;
}
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
QString macosName()
{
    // "Computer Name" from System Settings > General > Sharing, e.g. "Noah's MacBook Pro"
    QString name;
    if (CFStringRef computerName = SCDynamicStoreCopyComputerName(nullptr, nullptr)) {
        name = QString::fromCFString(computerName);
        CFRelease(computerName);
    }
    return name;
}
#elif defined(Q_OS_LINUX)
QString linuxPrettyHostname()
{
    // systemd's PRETTY_HOSTNAME (hostnamectl set-hostname --pretty)
    QFile file(QStringLiteral("/etc/machine-info"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith(QLatin1String("PRETTY_HOSTNAME="))) {
            QString value = line.mid(16);
            if (value.size() >= 2 && (value.startsWith('"') || value.startsWith('\'')) && value.endsWith(value.at(0))) {
                value = value.mid(1, value.size() - 2);
            }
            return value.trimmed();
        }
    }
    return QString();
}
#endif
} // namespace

namespace deviceName
{

QString platformName()
{
    QString name;
#if defined(Q_OS_ANDROID)
    name = androidName();
#elif defined(Q_OS_MACOS) && !defined(MACOS_NE)
    name = macosName();
#elif defined(Q_OS_LINUX)
    name = linuxPrettyHostname();
#endif
    if (!isUsable(name)) {
        name = QSysInfo::machineHostName();
        if (name.endsWith(QLatin1String(".local"), Qt::CaseInsensitive)) {
            name.chop(6);
        }
    }
    return isUsable(name) ? name.trimmed() : QString();
}

QString toConfigName(const QString &name, int maxLength)
{
    static const QHash<QChar, QString> translit = {
        { u'а', "a" },  { u'б', "b" },  { u'в', "v" },  { u'г', "g" },   { u'д', "d" },    { u'е', "e" },  { u'ё', "e" },
        { u'ж', "zh" }, { u'з', "z" },  { u'и', "i" },  { u'й', "y" },   { u'к', "k" },    { u'л', "l" },  { u'м', "m" },
        { u'н', "n" },  { u'о', "o" },  { u'п', "p" },  { u'р', "r" },   { u'с', "s" },    { u'т', "t" },  { u'у', "u" },
        { u'ф', "f" },  { u'х', "kh" }, { u'ц', "ts" }, { u'ч', "ch" },  { u'ш', "sh" },   { u'щ', "sch" }, { u'ъ', "" },
        { u'ы', "y" },  { u'ь', "" },   { u'э', "e" },  { u'ю', "yu" },  { u'я', "ya" },   { u'і', "i" },  { u'ї', "yi" },
        { u'є', "ye" }, { u'ґ', "g" },
    };
    static const QRegularExpression invalidChars(QStringLiteral("[^a-z0-9]+"));

    QString out;
    for (const QChar c : name.toLower()) {
        if (c == u'\'' || c == u'’' || c == u'`') {
            continue; // "Noah's MacBook" -> "noahs-macbook"
        }
        const auto it = translit.constFind(c);
        out += it != translit.constEnd() ? *it : QString(c);
    }
    out.replace(invalidChars, QStringLiteral("-"));
    while (out.startsWith('-')) {
        out.remove(0, 1);
    }
    out = out.left(maxLength);
    while (out.endsWith('-')) {
        out.chop(1);
    }
    return out;
}

} // namespace deviceName
