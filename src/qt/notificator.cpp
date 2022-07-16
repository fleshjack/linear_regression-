#include "notificator.h"

#include <QMetaType>
#include <QVariant>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QByteArray>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QImageWriter>

#ifdef USE_DBUS
#include <QtDBus>
#include <stdint.h>
#endif

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include "macnotificationhand