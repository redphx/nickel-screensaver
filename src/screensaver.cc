#include "screensaver.h"
#include <NickelHook.h>

#include <QtGlobal>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QDesktopWidget>
#include <QScreen>
#include <QDir>
#include <QTime>
#include <QPainter>
#include <QFile>
#include <QSettings>
#include <QFileInfo>

typedef void N3PowerWorkflowManager;
typedef void PowerViewController;
typedef QWidget BookCoverDragonPowerView;

constexpr const char* BOOK_COLOR_OVERLAY            = "Book/ColorOverlay";
constexpr const char* BOOK_COLOR_OVERLAY_ALPHA      = "Book/ColorOverlayAlpha";
constexpr const char* WALLPAPER_COLOR_OVERLAY       = "Wallpaper/ColorOverlay";
constexpr const char* WALLPAPER_COLOR_OVERLAY_ALPHA = "Wallpaper/ColorOverlayAlpha";

enum DISPLAY_MODE {
    None      = 0b00000,
    Overlay   = 0b00001,
    Book      = 0b00010,
    Wallpaper = 0b00100,
};

// Black 1x1 PNG file
unsigned char blank_screensaver[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00,
    0x0d, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x01, 0x02, 0x00, 0xfd, 0xff,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x53, 0x2b, 0x9c, 0x30, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

void (*N3PowerWorkflowManager_handleSleep)(N3PowerWorkflowManager* self);
void (*N3PowerWorkflowManager_showSleepView)(N3PowerWorkflowManager* self);

void* (*MainWindowController_sharedInstance)();
QWidget* (*MainWindowController_currentView)(void*);
void (*BookCoverDragonPowerView_setInfoPanelVisible)(BookCoverDragonPowerView* self, bool visible);
void (*FullScreenDragonPowerView_setImage)(QWidget* self, const QImage& img);
void (*FullScreenDragonPowerView_setInfoPanelVisible)(QWidget* self, bool visible);


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

    // Save to file
    settings.sync();
}

int ns_init() {
    // Only seed qsrand() once
    qsrand(QTime::currentTime().msec());

    // Setup folder structure
    bool has_wallpaper_dir = QDir("/mnt/onboard/.adds/screensaver/wallpaper").exists();
    QDir("/mnt/onboard/.adds/screensaver").mkpath("./wallpaper/overlay");
    if (!has_wallpaper_dir) {
        // Create "cover" file when "wallpaper" folder doesn't exist
        QFile cover("/mnt/onboard/.adds/screensaver/wallpaper/cover");
        cover.open(QIODevice::WriteOnly);
        cover.close();
    }

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
        .sym     = "_ZN22N3PowerWorkflowManager13showSleepViewEv", 
        .sym_new = "ns_show_sleep_view",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(N3PowerWorkflowManager_showSleepView),
        .desc    = "Show sleep view"
    },
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
		.out  = nh_symoutptr(MainWindowController_sharedInstance),
	},
	{
		.name = "_ZNK20MainWindowController11currentViewEv",
		.out  = nh_symoutptr(MainWindowController_currentView),
	},
    {
        .name = "_ZN25FullScreenDragonPowerView8setImageERK6QImage",
        .out  = nh_symoutptr(FullScreenDragonPowerView_setImage),
    },
    {
        .name = "_ZN24BookCoverDragonPowerView19setInfoPanelVisibleEb",
        .out  = nh_symoutptr(BookCoverDragonPowerView_setInfoPanelVisible),
        .desc = "",
        .optional = true,
    },
    {
        .name = "_ZN25FullScreenDragonPowerView19setInfoPanelVisibleEb",
        .out  = nh_symoutptr(FullScreenDragonPowerView_setInfoPanelVisible),
        .desc = "",
        .optional = true,
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

QImage screensaver_image;
bool is_cover_wallpaper = false;

QString pick_random_file(QDir dir, QStringList filters) {
    if (!dir.exists()) {
        return "";
    }

    QStringList files = dir.entryList(filters, QDir::Files);
    if (files.isEmpty()) {
        return "";
    }

    int idx = qrand() % files.size();
    return dir.filePath(files.at(idx));
}

bool write_blank_screensaver(const QString &file_path) {
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    qint64 written = file.write(reinterpret_cast<const char*>(blank_screensaver), sizeof(blank_screensaver));
    file.close();

    return written == sizeof(blank_screensaver);
}

extern "C" __attribute__((visibility("default")))
void ns_handle_sleep(N3PowerWorkflowManager* self) {
    // Reset data
    screensaver_image = QImage();
    is_cover_wallpaper = false;

    QString screensaver_path   = "/mnt/onboard/.adds/screensaver";
    QString kobo_screensaver_path = "/mnt/onboard/.kobo/screensaver";
    QDir screensaver_dir(screensaver_path);
    QDir kobo_screensaver_dir(kobo_screensaver_path);

    if (!kobo_screensaver_dir.exists()) {
        // Skip if Kobo's screensaver folder doesn't exist
        return N3PowerWorkflowManager_handleSleep(self);
    }

    void *mwc = MainWindowController_sharedInstance();
	if (!mwc) {
		nh_log("Invalid MainWindowController");
		return N3PowerWorkflowManager_handleSleep(self);
	}

    QWidget *current_view = MainWindowController_currentView(mwc);
	if (!current_view) {
		nh_log("Invalid currentView");
		return N3PowerWorkflowManager_handleSleep(self);
	}

    QString current_view_name = current_view->objectName();
    // Enable transparent mode when reading
    bool is_reading = current_view_name == QStringLiteral("ReadingView");

    // 1. Ensure folder structure
    screensaver_dir.mkpath("./wallpaper");
    QDir wallpaper_dir("/mnt/onboard/.adds/screensaver/wallpaper");
    QDir wallpaper_overlay_dir("/mnt/onboard/.adds/screensaver/wallpaper/overlay");

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

    int display_mode = is_reading ? DISPLAY_MODE::Book : DISPLAY_MODE::Wallpaper;

    // Pick a random overlay file
    QString random_file;
    if (is_reading) {
        random_file = pick_random_file(screensaver_dir, QStringList() << "*.png" << "*.jpg");
    } else {
        // Only accept PNG file in wallpaper's overlay folder
        random_file = pick_random_file(wallpaper_overlay_dir, QStringList() << "*.png");
    }
    if (!random_file.isEmpty()) {
        if (random_file.endsWith(".png")) {
            // Add Overlay mode
            display_mode |= DISPLAY_MODE::Overlay;
            overlay_file = random_file;
        } else {
            // To Wallpaper only mode
            display_mode = DISPLAY_MODE::Wallpaper;
            wallpaper_file = random_file;
        }
    }

    if ((display_mode & DISPLAY_MODE::Wallpaper) && wallpaper_file.isEmpty()) {
        // Find a random wallpaper in screensaver/wallpaper/
        QString random_file = pick_random_file(wallpaper_dir, QStringList() << "*.png" << "*.jpg" << "cover");
        if (random_file.isEmpty()) {
            if (overlay_file.isEmpty()) {
                // No overlay+wallpaper -> switch to None mode
                display_mode = DISPLAY_MODE::None;
            } else {
                // Has overlay but not wallpaper -> Set to overlay cover mode
                is_cover_wallpaper = true;
                display_mode &= ~DISPLAY_MODE::Wallpaper;
            }
        } else {
            if (random_file.endsWith("/cover")) {
                is_cover_wallpaper = true;
                display_mode &= ~DISPLAY_MODE::Wallpaper;
            } else {
                wallpaper_file = random_file;
            }
        }
    }

    if (display_mode == DISPLAY_MODE::None) {
        // Skip if no files found
        return N3PowerWorkflowManager_handleSleep(self);
    }

    // Write Tiny PNG
    if (!is_cover_wallpaper) {
        write_blank_screensaver("/mnt/onboard/.kobo/screensaver/nickel-screensaver.png");
    }

    // If not overlay mode -> only load the wallpaper file
    if (!(display_mode & DISPLAY_MODE::Overlay)) {
        if (!wallpaper_file.isEmpty()) {
            screensaver_image.load(wallpaper_file);
        }

        return N3PowerWorkflowManager_handleSleep(self);
    }

    // 5. Handle transparent mode
    QPixmap wallpaper;

    QDesktopWidget* desktop_widget = QApplication::desktop();
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screen_size = screen->size();

    if (display_mode & DISPLAY_MODE::Book) {
        // Take screenshot of the current screen if reading
        QRect geometry = current_view->geometry();
        wallpaper = screen->grabWindow(
            desktop_widget->winId(),
            geometry.left(),
            geometry.top(),
            geometry.width(),
            geometry.height()
        );
    } else if (display_mode & DISPLAY_MODE::Wallpaper and !wallpaper_file.isEmpty()) {
        wallpaper.load(wallpaper_file);
    }

    // 6. Combine overlay & wallpaper
    QImage result(screen_size, is_cover_wallpaper ? QImage::Format_ARGB32 : QImage::Format_RGB32);
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
        if (!overlay.isNull()) {
            if (overlay.size() != screen_size) {
                // Only scales if different sizes
                painter.drawPixmap(0, 0, overlay.scaled(screen_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
            } else {
                painter.drawPixmap(0, 0, overlay);
            }
        }
    }
    painter.end();

    // 7. Save screensaver
    screensaver_image = result;

    // 8. Done
    N3PowerWorkflowManager_handleSleep(self);
    // nh_log("Current view: %s", current_view_name.toStdString().c_str());
    // nh_dump_log();
}

extern "C" __attribute__((visibility("default")))
void ns_show_sleep_view(N3PowerWorkflowManager* self) {
    N3PowerWorkflowManager_showSleepView(self);

    // Remove blank screensaver
    QFile file("/mnt/onboard/.kobo/screensaver/nickel-screensaver.png");
    file.remove();

    if (screensaver_image.isNull()) {
        return;
    }

    void *mwc = MainWindowController_sharedInstance();
    QWidget *current_view = MainWindowController_currentView(mwc);

    // Check if cover mode
    if (is_cover_wallpaper) {
        QPixmap pixmap = QPixmap::fromImage(screensaver_image);
        QLabel* overlay = new QLabel(current_view);
        overlay->setPixmap(pixmap);
        overlay->setGeometry(current_view->rect());
        overlay->lower();
        overlay->show();
    } else {
        // Replace current image with the generated screensaver
        FullScreenDragonPowerView_setImage(current_view, screensaver_image);
    }


    // BookCoverDragonPowerView_setInfoPanelVisible(current_view, true);
}
