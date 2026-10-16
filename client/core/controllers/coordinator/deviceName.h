#ifndef DEVICENAME_H
#define DEVICENAME_H

#include <QString>

// Human-readable name of this device for coordinator config names and device labels (FORK.md, coordinator client).
namespace deviceName
{
// What the platform calls this device: the name from Android settings or manufacturer + model, the macOS computer name,
// the Linux pretty hostname, else the hostname. Empty when nothing better than "localhost" is known.
QString platformName();

// name as a coordinator config name: Cyrillic transliterated, lowercase [a-z0-9-], at most maxLength characters.
// Empty when nothing usable is left.
QString toConfigName(const QString &name, int maxLength = 32);
} // namespace deviceName

#endif // DEVICENAME_H
