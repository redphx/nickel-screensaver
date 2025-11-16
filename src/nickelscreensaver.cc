#include "nickelscreensaver.h"
#include <NickelHook.h>

#include <QtGlobal>
#include <QApplication>
#include <QWidget>
#include <QDesktopWidget>
#include <QScreen>
#include <QDir>
#include <QTime>
#include <QPainter>
#include <QFile>
#include <QSettings>
#include <QFileInfo>

typedef void N3PowerWorkflowManager;

constexpr const char* BOOK_COLOR_OVERLAY            = "Book/ColorOverlay";
constexpr const char* BOOK_COLOR_OVERLAY_ALPHA      = "Book/ColorOverlayAlpha";
constexpr const char* WALLPAPER_COLOR_OVERLAY       = "Wallpaper/ColorOverlay";
constexpr const char* WALLPAPER_COLOR_OVERLAY_ALPHA = "Wallpaper/ColorOverlayAlpha";
constexpr const char* WALLPAPER_SHOW_IMAGE_OVERLAY  = "Wallpaper/ShowImageOverlay";

enum DISPLAY_MODE {
    None      = 0b00000,
    Overlay   = 0b00001,
    Book      = 0b00010,
    Wallpaper = 0b00100,
};

void (*N3PowerWorkflowManager_handleSleep)(N3PowerWorkflowManager *_this);
void *(*MainWindowController_sharedInstance)();
QWidget *(*MainWindowController_currentView)(void *);

struct nh_info nickelscreensaver = {
    .name = "Nickel Screensaver",
    .desc = "Transparent screensaver support for Kobo",
    .uninstall_flag = NICKEL_SCREENSAVER_DELETE_FILE,
};

void save_settings(QSettings &settings) {
    QString book_color_overlay = settings.value(BOOK_COLOR_OVERLAY, "ffffff").toString();
    settings.setValue(BOOK_COLOR_OVERLAY, book_color_overlay);

    int book_color_overlay_alpha = qBound(0, settings.value(BOOK_COLOR_OVERLAY_ALPHA, 0).toInt(), 100);
    settings.setValue(BOOK_COLOR_OVERLAY_ALPHA, book_color_overlay_alpha);

    QString wallpaper_color_overlay = settings.value(WALLPAPER_COLOR_OVERLAY, "ffffff").toString();
    settings.setValue(WALLPAPER_COLOR_OVERLAY, wallpaper_color_overlay);

    int wallpaper_color_overlay_alpha = qBound(0, settings.value(WALLPAPER_COLOR_OVERLAY_ALPHA, 0).toInt(), 100);
    settings.setValue(WALLPAPER_COLOR_OVERLAY_ALPHA, wallpaper_color_overlay_alpha);

    bool wallpaper_show_image_overlay = settings.value(WALLPAPER_SHOW_IMAGE_OVERLAY, true).toBool();
    settings.setValue(WALLPAPER_SHOW_IMAGE_OVERLAY, wallpaper_show_image_overlay);

    // Save to file
    settings.sync();
}

int ns_init() {
    // Only seed qsrand() once
    qsrand(QTime::currentTime().msec());

    // Setup folder structure
    QDir("/mnt/onboard/.adds/screensaver").mkpath("./wallpaper");

    // Setup settings
    QSettings settings("/mnt/onboard/.adds/screensaver/_settings.ini", QSettings::IniFormat);
    save_settings(settings);

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

QString pick_random_file(QDir dir, QStringList filters) {
    QStringList files = dir.entryList(filters, QDir::Files);
    if (files.isEmpty()) {
        return "";
    }

    int idx = qrand() % files.size();
    return dir.filePath(files.at(idx));
}

extern "C" __attribute__((visibility("default"))) void ns_handle_sleep(N3PowerWorkflowManager *_this) {
    QString screensaver_path   = "/mnt/onboard/.adds/screensaver";
    QString kobo_screensaver_path = "/mnt/onboard/.kobo/screensaver";
    QDir screensaver_dir(screensaver_path);
    QDir kobo_screensaver_dir(kobo_screensaver_path);

    if (!kobo_screensaver_dir.exists()) {
        // Skip if Kobo's screensaver folder doesn't exist
        return N3PowerWorkflowManager_handleSleep(_this);
    }

    void *mwc = MainWindowController_sharedInstance();
	if (!mwc) {
		nh_log("Invalid MainWindowController");
		return N3PowerWorkflowManager_handleSleep(_this);
	}

    QWidget *current_view = MainWindowController_currentView(mwc);
	if (!current_view) {
		nh_log("Invalid currentView");
		return N3PowerWorkflowManager_handleSleep(_this);
	}

    QString current_view_name = current_view->objectName();
    // Enable transparent mode when reading
    bool is_reading = current_view_name == QStringLiteral("ReadingView");
    int display_mode = DISPLAY_MODE::None;

    // 1. Ensure folder structure
    screensaver_dir.mkpath("./wallpaper");
    QDir wallpaper_dir("/mnt/onboard/.adds/screensaver/wallpaper");

    // 2. Move old overlay_files from .kobo/screensaver to .adds/screensaver
    QStringList exclude = {
        "_settings.ini",
        "nickel-screensaver.png",
        "nickel-screensaver.jpg",
    };
    for (const QFileInfo &file : kobo_screensaver_dir.entryInfoList(QDir::Files)) {
        // Don't move Nickel Screensaver's overlay_files
        if (exclude.contains(file.fileName())) {
            continue;
        }

        QString dest_path = screensaver_path + '/' + file.fileName();
        // Don't override file with the same name in .adds/screensaver
        if (!QFile::exists(dest_path)) {
            QFile::rename(file.filePath(), dest_path);
        }
    }

    // 3. Empty .kobo/screensaver folder
    kobo_screensaver_dir.removeRecursively();
    kobo_screensaver_dir.mkpath(".");

    // Get settings
    QSettings settings("/mnt/onboard/.adds/screensaver/_settings.ini", QSettings::IniFormat);

    // 4. Pick a random overlay
    QStringList overlay_files;
    QStringList wallpaper_files;

    QString overlay_file;
    QString wallpaper_file;

    if (is_reading) {
        // Check for PNG overlays first
        QString random_file = pick_random_file(screensaver_dir, QStringList() << "*.png");
        if (random_file.isEmpty()) {
            random_file = pick_random_file(screensaver_dir, QStringList() << "*.jpg");
            if (!random_file.isEmpty()) {
                display_mode |= DISPLAY_MODE::Wallpaper;
                wallpaper_file = random_file;
            }
        } else {
            display_mode |= DISPLAY_MODE::Overlay | DISPLAY_MODE::Book;
            overlay_file = random_file;
        }
    } else if (settings.value(WALLPAPER_SHOW_IMAGE_OVERLAY, true).toBool()) {
        QString random_file = pick_random_file(screensaver_dir, QStringList() << "*.png" << "*.jpg");
        if (!random_file.isEmpty()) {
            if (random_file.endsWith(".png")) {
                display_mode |= DISPLAY_MODE::Overlay;
                overlay_file = random_file;
            } else {
                display_mode |= DISPLAY_MODE::Wallpaper;
                wallpaper_file = random_file;
            }
        }
    }

    if (!is_reading && wallpaper_file.isEmpty()) {
        // Find random wallpaper
        QString random_file = pick_random_file(wallpaper_dir, QStringList() << "*.png" << "*.jpg");
        if (!random_file.isEmpty()) {
            display_mode |= DISPLAY_MODE::Wallpaper;
            wallpaper_file = random_file;
        } else {
            display_mode &= ~DISPLAY_MODE::Wallpaper;
        }
    }

    if (display_mode == DISPLAY_MODE::None) {
        // Skip if no files found
        return N3PowerWorkflowManager_handleSleep(_this);
    }

    // If not overlay mode -> copy the file to .kobo/screensaver
    if (!(display_mode & DISPLAY_MODE::Overlay)) {
        if (!wallpaper_file.isEmpty()) {
            QFileInfo info(wallpaper_file);
            QFile::copy(wallpaper_file, kobo_screensaver_path + "/nickel-screensaver." + info.suffix());
        }
        N3PowerWorkflowManager_handleSleep(_this);
        return;
    }

    // 5. Handle transparent mode
    QPixmap wallpaper;

    QDesktopWidget* desktopWidget = QApplication::desktop();
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screen_size = screen->size();

    if (display_mode & DISPLAY_MODE::Book) {
        // Take screenshot of the current screen if reading
        QRect geometry = current_view->geometry();
        wallpaper = screen->grabWindow(
            desktopWidget->winId(),
            geometry.left(),
            geometry.top(),
            geometry.width(),
            geometry.height()
        );
    } else if (display_mode & DISPLAY_MODE::Wallpaper and !wallpaper_file.isEmpty()) {
        wallpaper.load(wallpaper_file);
    }

    // 6. Combine overlay & wallpaper
    QImage result(screen_size, QImage::Format_RGB32);
    QPainter painter(&result);

    // Draw wallpaper
    if (!wallpaper.isNull()) {
        if (wallpaper.size() != screen_size) {
            // Only scales if different sizes
            painter.drawPixmap(0, 0, wallpaper.scaled(screen_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
        } else {
            painter.drawPixmap(0, 0, wallpaper);
        }
    }

    // Draw color overlay layer
    QColor color_overlay;
    QString color_overlay_hex = settings.value(is_reading ? BOOK_COLOR_OVERLAY : WALLPAPER_COLOR_OVERLAY, "ffffff").toString();
    int color_overlay_alpha = settings.value(is_reading ? BOOK_COLOR_OVERLAY_ALPHA : WALLPAPER_COLOR_OVERLAY_ALPHA, 0).toInt();
    color_overlay_alpha = qBound(0, color_overlay_alpha, 100);

    if (!color_overlay_hex.isEmpty() && color_overlay_alpha > 0) {
        color_overlay.setNamedColor("#" + color_overlay_hex);
        if (color_overlay.isValid()) {
            color_overlay.setAlpha(color_overlay_alpha * 255 / 100);
            painter.fillRect(0, 0, screen_size.width(), screen_size.height(), color_overlay);
        }
    }

    // Draw image overlay
    QPixmap overlay;
    if (!overlay_file.isEmpty()) {
        overlay.load(overlay_file);

        if (overlay.size() != screen_size) {
            // Only scales if different sizes
            painter.drawPixmap(0, 0, overlay.scaled(screen_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
        } else {
            painter.drawPixmap(0, 0, overlay);
        }
    }
    painter.end();

    // 7. Save screensaver
    result.save(kobo_screensaver_path + "/nickel-screensaver.jpg", "JPEG", 100);

    // 8. Done
    N3PowerWorkflowManager_handleSleep(_this);
    // nh_log("Current view: %s", current_view_name.toStdString().c_str());
    // nh_dump_log();
}
