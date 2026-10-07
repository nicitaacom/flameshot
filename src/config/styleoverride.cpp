// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2020 Jeremy Borgman <borgman.jeremy@pm.me>
//
// Created by jeremy on 9/24/20.

#include "styleoverride.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QStandardPaths>

int StyleOverride::styleHint(StyleHint hint,
                             const QStyleOption* option,
                             const QWidget* widget,
                             QStyleHintReturn* returnData) const
{
    if (hint == SH_ToolTip_WakeUpDelay) {
        return 600;
    } else {
        return baseStyle()->styleHint(hint, option, widget, returnData);
    }
}

void watchCustomStyleSheet()
{
    if (qApp->findChild<QFileSystemWatcher*>("customThemeWatcher")) {
        return;
    }
    const QString directory =
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
      "/flameshot-theme";
    const QString path = directory + "/current.qss";
    if (!QDir().mkpath(directory)) {
        return;
    }
    auto* watcher = new QFileSystemWatcher(qApp);
    watcher->setObjectName("customThemeWatcher");
    watcher->addPath(directory);
    const auto apply = [watcher, path]() {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
            // Atomic replacements remove the old file from the watch list.
            if (!watcher->files().contains(path)) {
                watcher->addPath(path);
            }
        }
    };
    QObject::connect(watcher, &QFileSystemWatcher::fileChanged, qApp, apply);
    QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, qApp, apply);
    apply();
}
