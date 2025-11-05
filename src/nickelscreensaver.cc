#include "nickelscreensaver.h"
#include <NickelHook.h>

#include <QApplication>
#include <QWidget>
#include <QDesktopWidget>
#include <QScreen>
#include <QDir>
#include <QTime>
#include <QPainter>
#include <QFile>
#include <QFileInfo>

typedef void N3PowerWorkflowManager;

void (*N3PowerWorkflowManager_handleSleep)(N3PowerWorkflowManager *_this);
void *(*MainWindowController_sharedInstance)();
QWidget *(*MainWindowController_currentView)(void *);

struct nh_info nickelscreensaver = {
    .name = "Nickel Screensaver",
    .desc = "Transparent screensaver support for Kobo",
    .uninstall_flag = NICKEL_SCREENSAVER_DELETE_FILE,
};

int ns_init() {
    return 0;
}

bool ns_uninstall() {
    return true;
}

struct nh_hook nickelscreensaverHook[] = {
    {
        .sym     = "_ZN22N3PowerWorkflowManager11handleSleepEv", 
        .sym_new = "ns_handle_sleep",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(N3PowerWorkflowManager_handleSleep),
        .desc    = "Handle sleep"
    },
    {0}
};

struct nh_dlsym nickelscreensaverDlsym[] = {
    {
		.name = "_ZN20MainWindowController14sharedInstanceEv",
		.out = nh_symoutptr(MainWindowController_sharedInstance)
	},
	{
		.name = "_ZNK20MainWindowController11currentViewEv",
		.out = nh_symoutptr(MainWindowController_currentView)
	},
	{0}
};

NickelHook(
    .init      = &ns_init,
    .info      = &nickelscreensaver,
    .hook      = nickelscreensaverHook,
    .dlsym     = nickelscreensaverDlsym,
    .uninstall = &ns_uninstall,
);

extern "C" __attribute__((visibility("default"))) void ns_handle_sleep(N3PowerWorkflowManager *_this) {
    QString ns_path   = "/mnt/onboard/.adds/screensaver";
    QString kobo_path = "/mnt/onboard/.kobo/screensaver";
    QDir ns_dir(ns_path);
    QDir kobo_dir(kobo_path);

    if (!kobo_dir.exists()) {
        // Skip if Kobo's screensaver folder doesn't exist
        return N3PowerWorkflowManager_handleSleep(_this);
    }

    QString current_view_name = QString();

    void *mwc = MainWindowController_sharedInstance();
	if (!mwc) {
		nh_log("invalid MainWindowController");
		return N3PowerWorkflowManager_handleSleep(_this);
	}

    QWidget *current_view = MainWindowController_currentView(mwc);
	if (!current_view) {
		nh_log("invalid View");
		return N3PowerWorkflowManager_handleSleep(_this);
	}

    current_view_name = current_view->objectName();
    // Enable transparent mode when reading
    bool transparent_mode = current_view_name == QStringLiteral("ReadingView");

    // 1. Check NS's folder
    if (!ns_dir.exists()) {
        // Create empty screensaver folder
        ns_dir.mkpath(".");
    }

    // 2. Move old files from .kobo/screensaver to .adds/screensaver
    QStringList exclude = {
        "_config.ini",
        "nickel-screensaver.png",
        "nickel-screensaver.jpg",
    };
    for (const QFileInfo &file : kobo_dir.entryInfoList(QDir::Files)) {
        // Don't move Nickel Screensaver's files
        if (exclude.contains(file.fileName())) {
            continue;
        }

        QString dest_path = ns_path + '/' + file.fileName();
        // Don't override file with the same name in .adds/screensaver
        if (!QFile::exists(dest_path)) {
            QFile::rename(file.filePath(), dest_path);
        }
    }

    // 3. Empty .kobo/screensaver folder
    kobo_dir.removeRecursively();
    kobo_dir.mkpath(".");

    // 4. Pick a random screensaver
    QStringList files;
    // Only accept PNG files in transparent mode
    if (transparent_mode) {
        files = ns_dir.entryList(QStringList() << "*.png", QDir::Files);

        // If there is no PNG files -> check for JPG files, and switch to non-transparent mode
        if (files.isEmpty()) {
            files = ns_dir.entryList(QStringList() << "*.jpg", QDir::Files);
            transparent_mode = false;
        }
    } else {
        files = ns_dir.entryList(QStringList() << "*.png" << "*.jpg", QDir::Files);
    }

    if (files.isEmpty()) {
        // Skip if no files found
        return N3PowerWorkflowManager_handleSleep(_this);
    }

    // Seed qrand
    qsrand(QTime::currentTime().msec());
    int idx = qrand() % files.size();
    QString random_file = ns_dir.filePath(files.at(idx));

    // If not transparent mode -> copy the file to .kobo/screensaver
    if (!transparent_mode) {
        QFileInfo info(random_file);
        QFile::copy(random_file, kobo_path + "/nickel-screensaver." + info.suffix());
        N3PowerWorkflowManager_handleSleep(_this);
        return;
    }

    // Handle transparent mode
    // 5. Take screenshot of the current screen
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect geometry = current_view->geometry();
    QScreen* screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(
        desktopWidget->winId(),
        geometry.left(),
        geometry.top(),
        geometry.width(),
        geometry.height()
    );

    // 6. Combine
    QPixmap overlay(random_file);
    QImage result(screenshot.size(), QImage::Format_RGB32);
    QPainter painter(&result);
    painter.drawPixmap(0, 0, screenshot);
    if (overlay.size() != screenshot.size()) {
        // Only scales if different sizes
        painter.drawPixmap(0, 0, overlay.scaled(screenshot.size(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
    } else {
        painter.drawPixmap(0, 0, overlay);
    }
    painter.end();

    // 7. Save screensaver
    result.save(kobo_path + "/nickel-screensaver.jpg", "JPEG", 100);

    // 8. Done
    N3PowerWorkflowManager_handleSleep(_this);
    // nh_log("Current view: %s", current_view_name.toStdString().c_str());
    // nh_dump_log();
}
