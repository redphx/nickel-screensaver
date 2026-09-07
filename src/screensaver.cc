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
#include <QBuffer>
#include <QImageReader>
#include <QTimer>
#include <QFont>
#include <QEvent>
#include <cstdint>

typedef void N3PowerWorkflowManager;
typedef void PowerViewController;
typedef QWidget BookCoverDragonPowerView;
typedef QObject HardwareInterface;

constexpr const char* BOOK_COLOR_OVERLAY            = "Book/ColorOverlay";
constexpr const char* BOOK_COLOR_OVERLAY_ALPHA      = "Book/ColorOverlayAlpha";
constexpr const char* WALLPAPER_COLOR_OVERLAY       = "Wallpaper/ColorOverlay";
constexpr const char* WALLPAPER_COLOR_OVERLAY_ALPHA = "Wallpaper/ColorOverlayAlpha";

constexpr const char* GLITCH_ENABLED    = "Glitch/Enabled";
constexpr const char* GLITCH_ITERATIONS = "Glitch/Iterations";
constexpr const char* GLITCH_QUALITY    = "Glitch/Quality";

constexpr const char* BATTERY_ENABLED = "Battery/Enabled";

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
void (*N3PowerWorkflowManager_powerOff)(N3PowerWorkflowManager* self, bool low_battery);
void (*N3PowerWorkflowManager_showPowerOffView)(N3PowerWorkflowManager* self);

void* (*MainWindowController_sharedInstance)();
QWidget* (*MainWindowController_currentView)(void*);
void (*BookCoverDragonPowerView_setInfoPanelVisible)(BookCoverDragonPowerView* self, bool visible);
void (*FullScreenDragonPowerView_setImage)(QWidget* self, const QImage& img);
void (*FullScreenDragonPowerView_setInfoPanelVisible)(QWidget* self, bool visible);
HardwareInterface* (*HardwareFactory_sharedInstance)();
std::uintptr_t** HardwareInterface_vtable;
uint (*HardwareInterface_chargingState)(HardwareInterface* self);


struct nh_info nickelscreensaver = {
    .name = "Nickel Screensaver",
    .desc = "Transparent screensaver support for Kobo",
    .uninstall_flag = NICKEL_SCREENSAVER_DELETE_FILE,
};

void save_settings(QSettings &settings) {
    // Book
    QString book_color_overlay = settings.value(BOOK_COLOR_OVERLAY, "ffffff").toString();
    settings.setValue(BOOK_COLOR_OVERLAY, book_color_overlay);

    int book_color_overlay_alpha = qBound(0, settings.value(BOOK_COLOR_OVERLAY_ALPHA, 0).toInt(), 100);
    settings.setValue(BOOK_COLOR_OVERLAY_ALPHA, book_color_overlay_alpha);

    // Wallpaper
    QString wallpaper_color_overlay = settings.value(WALLPAPER_COLOR_OVERLAY, "ffffff").toString();
    settings.setValue(WALLPAPER_COLOR_OVERLAY, wallpaper_color_overlay);

    int wallpaper_color_overlay_alpha = qBound(0, settings.value(WALLPAPER_COLOR_OVERLAY_ALPHA, 0).toInt(), 100);
    settings.setValue(WALLPAPER_COLOR_OVERLAY_ALPHA, wallpaper_color_overlay_alpha);

    // Glitch
    bool glitch_enabled = settings.value(GLITCH_ENABLED, false).toBool();
    settings.setValue(GLITCH_ENABLED, glitch_enabled);

    int glitch_iterations = qBound(2, settings.value(GLITCH_ITERATIONS, 5).toInt(), 10);
    settings.setValue(GLITCH_ITERATIONS, glitch_iterations);

    int glitch_quality = qBound(10, settings.value(GLITCH_QUALITY, 10).toInt(), 100);
    settings.setValue(GLITCH_QUALITY, glitch_quality);

    // Battery
    bool battery_enabled = settings.value(BATTERY_ENABLED, true).toBool();
    settings.setValue(BATTERY_ENABLED, battery_enabled);

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
        .sym      = "_ZN22N3PowerWorkflowManager13showSleepViewEv",
        .sym_new  = "hook_N3PowerWorkflowManager_showSleepView",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(N3PowerWorkflowManager_showSleepView),
        .desc     = "Show Sleep view",
        .optional = true,
    },
    {
        .sym      = "_ZN22N3PowerWorkflowManager11handleSleepEv",
        .sym_new  = "hook_N3PowerWorkflowManager_handleSleep",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(N3PowerWorkflowManager_handleSleep),
        .desc     = "Handle sleep",
        .optional = true,
    },
    {
        .sym      = "_ZN22N3PowerWorkflowManager16showPowerOffViewEv",
        .sym_new  = "hook_N3PowerWorkflowManager_showPowerOffView",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(N3PowerWorkflowManager_showPowerOffView),
        .desc     = "Show Power Off view",
        .optional = true,
    },
    {
        .sym      = "_ZN22N3PowerWorkflowManager8powerOffEb",
        .sym_new  = "hook_N3PowerWorkflowManager_powerOff",
        .lib      = "libnickel.so.1.0.0",
        .out      = nh_symoutptr(N3PowerWorkflowManager_powerOff),
        .desc     = "Handle Power off",
        .optional = true,
    },
    {0},
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
	{
		.name = "_ZN15HardwareFactory14sharedInstanceEv",
		.out  = nh_symoutptr(HardwareFactory_sharedInstance),
		.desc = "HardwareFactory::sharedInstance()",
		.optional = true,
	},
	{
		.name = "_ZTV17HardwareInterface",
		.out  = nh_symoutptr(HardwareInterface_vtable),
		.desc = "HardwareInterface::vtable",
		.optional = true,
	},
	{
		.name = "_ZN17HardwareInterface13chargingStateEv",
		.out  = nh_symoutptr(HardwareInterface_chargingState),
		.desc = "HardwareInterface::chargingState()",
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

// Note: QImage and QPixmap are ref-counted, backing data is COW if more than one reference
QImage screensaver_image;
QPixmap overlay_pixmap;
bool is_overlay_wallpaper = false;

QByteArray read_power_supply_value(const QString &path, const QString &name) {
    QFile file(path + '/' + name);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    return file.readAll().trimmed();
}

qint64 read_power_supply_number(const QString &path, const QString &name) {
    bool ok = false;
    qint64 value = read_power_supply_value(path, name).toLongLong(&ok);
    return ok ? value : -1;
}

QString find_battery_path() {
    static const QStringList paths = {
        "/sys/class/power_supply/battery",
        "/sys/class/power_supply/bd71827_bat",
        "/sys/class/power_supply/mc13892_bat",
    };

    for (const QString &path : paths) {
        if (QFileInfo(path + "/capacity").exists() && QFileInfo(path + "/status").exists()) {
            return path;
        }
    }

    return "";
}

template <typename F>
F get_derived_hardware_method(F interface_method, HardwareInterface *hardware) {
    struct VPtr {
        std::uintptr_t **v;
    };

    if (!interface_method || !hardware || !HardwareInterface_vtable) {
        return nullptr;
    }

    std::uintptr_t **interface_vtable = HardwareInterface_vtable + 2;
    for (int offset = 0; offset < 64; ++offset) {
        if (interface_vtable[offset] == reinterpret_cast<std::uintptr_t*>(interface_method)) {
            VPtr *derived = reinterpret_cast<VPtr*>(hardware);
            return reinterpret_cast<F>(derived->v[offset]);
        }
    }

    return nullptr;
}

qint64 charging_minutes_remaining(const QString &path) {
    qint64 seconds = read_power_supply_number(path, "time_to_full_now");
    if (seconds < 0) {
        return -1;
    }

    return (seconds + 59) / 60;
}

QString format_duration(qint64 minutes) {
    if (minutes < 60) {
        return QStringLiteral("%1m").arg(minutes);
    }

    qint64 hours = minutes / 60;
    qint64 remaining_minutes = minutes % 60;
    if (remaining_minutes == 0) {
        return QStringLiteral("%1h").arg(hours);
    }

    return QStringLiteral("%1h %2m").arg(hours).arg(remaining_minutes);
}

ChargingOverlay::ChargingOverlay(QWidget *parent)
    : QLabel(parent),
      battery_path(find_battery_path()),
      refresh_timer(),
      hardware(nullptr),
      charging_state(nullptr) {
    setObjectName(QStringLiteral("nickelChargingOverlay"));
    setAlignment(Qt::AlignCenter);
    parent->installEventFilter(this);
    update_geometry();

    QFont overlay_font = font();
    overlay_font.setPixelSize(qMax(20, parent->width() / 32));
    overlay_font.setBold(true);
    setFont(overlay_font);
    setStyleSheet(QStringLiteral(
        "color: black; background-color: rgba(255, 255, 255, 190); padding: 8px;"));

    refresh_timer.setInterval(60 * 1000);
    refresh_timer.setTimerType(Qt::PreciseTimer);
    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(refresh()));

    hardware = HardwareFactory_sharedInstance ? HardwareFactory_sharedInstance() : nullptr;
    if (hardware) {
        charging_state = get_derived_hardware_method(HardwareInterface_chargingState, hardware);
        bool signals_connected = true;
        if (!connect(hardware, SIGNAL(usb_plugged()), this, SLOT(refresh()))) {
            signals_connected = false;
        }
        if (!connect(hardware, SIGNAL(usb_ac_plugged()), this, SLOT(refresh()))) {
            signals_connected = false;
        }
        if (!connect(hardware, SIGNAL(usb_unplugged()), this, SLOT(usb_unplugged()))) {
            signals_connected = false;
        }
        if (!connect(hardware, SIGNAL(usb_ac_unplugged()), this, SLOT(usb_unplugged()))) {
            signals_connected = false;
        }
        if (!connect(hardware, SIGNAL(battery_changed()), this, SLOT(refresh()))) {
            signals_connected = false;
        }
        if (!connect(hardware, SIGNAL(battery_level(int)), this, SLOT(refresh()))) {
            signals_connected = false;
        }
        if (!signals_connected) {
            nh_log("Could not connect all charging overlay USB signals");
        }
    }

    refresh();
}

bool ChargingOverlay::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        update_geometry();
    }

    return QLabel::eventFilter(watched, event);
}

void ChargingOverlay::update_geometry() {
    QWidget *view = parentWidget();
    if (!view) {
        return;
    }

    setGeometry(view->width() / 10, view->height() / 30,
                view->width() * 8 / 10, view->height() / 14);
}

void ChargingOverlay::refresh() {
    if (!hardware || !charging_state) {
        refresh_timer.stop();
        hide();
        return;
    }

    if (charging_state(hardware) == 0) {
        refresh_timer.stop();
        hide();
        return;
    }

    if (!refresh_timer.isActive()) {
        refresh_timer.start();
    }

    if (battery_path.isEmpty()) {
        battery_path = find_battery_path();
    }
    if (battery_path.isEmpty()) {
        hide();
        return;
    }

    qint64 capacity = read_power_supply_number(battery_path, "capacity");
    if (capacity < 0) {
        hide();
        return;
    }

    QByteArray status;
    if (capacity < 100) {
        status = read_power_supply_value(battery_path, "status");
    }

    QString text;
    if (status == "Full" || capacity >= 100) {
        text = QStringLiteral("%1% | Fully charged").arg(capacity);
    } else if (status == "Charging") {
        text = QStringLiteral("%1% | Charging").arg(capacity);
        qint64 minutes = charging_minutes_remaining(battery_path);
        if (minutes >= 0) {
            text += QStringLiteral(" | %1 until full").arg(format_duration(minutes));
        }
    } else {
        text = QStringLiteral("%1% | Plugged in").arg(capacity);
    }

    if (text != this->text()) {
        setText(text);
    }
    if (!isVisible()) {
        raise();
        show();
    }
}

void ChargingOverlay::usb_unplugged() {
    refresh_timer.stop();
    hide();
}

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

QImage glitch_image(const QImage& source, int iterations, int quality = 90) {
    // 1. Convert QImage to JPEG byte array
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    source.save(&buffer, "JPG", quality);

    // 2. Find the Start of Scan (SOS) marker: 0xFF 0xDA
    const uchar* data = (const uchar*)ba.constData();
    int img_size = ba.size();
    int scan_start = 0;
    for (int i = 0; i < img_size - 4; ++i) {
        if (data[i] == 0xFF && data[i + 1] == 0xDA) {
            // Big-endian: high byte at i+2, low byte at i+3
            int sos_header_length = (data[i + 2] << 8) | data[i + 3];
            scan_start = i + 2 + sos_header_length;
            break;
        }
    }

    if (scan_start == 0 || scan_start >= img_size - 2) {
        return source;
    }

    // 3. Apply the glitch logic
    iterations = qMax(1, iterations);

    static int noise_length = 97;  // prime number
    static QByteArray noise(noise_length, (char)1);
    int data_size = img_size - scan_start - 2;  // Ignore EOI (End of Image), which is 2 bytes: 0xFFD9
    int safe_range = data_size - noise_length;
    if (safe_range <= 0) {
        return source;
    }

    for (int i = 0; i < iterations; ++i) {
        int glitch_index = scan_start + (qrand() % safe_range);
        ba.replace(glitch_index, noise_length, noise);
    }

    QImage glitched;
    if (!glitched.loadFromData(ba, "JPG")) {
        return source;
    }

    return glitched;
}

QImage glitch_pixmap(const QPixmap& source, int iterations, int quality = 90) {
    QImage img = source.toImage();
    return glitch_image(img, iterations, quality);
}

QImage load_scaled_image(const QString& file_path, QSize screen_size) {
    QImageReader reader(file_path);
    if (reader.canRead()) {
        QSize img_size = reader.size();

        if (img_size != screen_size) {
            img_size.scale(screen_size, Qt::KeepAspectRatioByExpanding);
            reader.setScaledSize(img_size);
        }

        return reader.read();
    }

    return QImage();
}

void before_handle(N3PowerWorkflowManager* self) {
    // Reset data
    screensaver_image = QImage();
    is_overlay_wallpaper = false;

    QString screensaver_path   = "/mnt/onboard/.adds/screensaver";
    QString kobo_screensaver_path = "/mnt/onboard/.kobo/screensaver";
    QDir screensaver_dir(screensaver_path);
    QDir kobo_screensaver_dir(kobo_screensaver_path);

    if (!kobo_screensaver_dir.exists()) {
        // Skip if Kobo's screensaver folder doesn't exist
        return;
    }

    void *mwc = MainWindowController_sharedInstance();
	if (!mwc) {
		nh_log("Invalid MainWindowController");
		return;
	}

    QWidget *current_view = MainWindowController_currentView(mwc);
	if (!current_view) {
		nh_log("Invalid currentView");
		return;
	}

    QString current_view_name = current_view->objectName();

    // Don't show overlay again when the current view is Sleep view
    if (current_view_name.contains("DragonPowerView")) {
        return;
    }

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

    QDesktopWidget* desktop_widget = QApplication::desktop();
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screen_size = screen->size();

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
                is_overlay_wallpaper = true;
                display_mode &= ~DISPLAY_MODE::Wallpaper;
            }
        } else {
            if (random_file.endsWith("/cover")) {
                is_overlay_wallpaper = true;
                display_mode &= ~DISPLAY_MODE::Wallpaper;
            } else {
                wallpaper_file = random_file;
            }
        }
    }

    if (display_mode == DISPLAY_MODE::None) {
        // Skip if no files found
        return;
    }

    // Write Tiny PNG
    if (!is_overlay_wallpaper) {
        write_blank_screensaver("/mnt/onboard/.kobo/screensaver/nickel-screensaver.png");
    }

    // If not overlay mode -> only load the wallpaper file
    if (!(display_mode & DISPLAY_MODE::Overlay)) {
        if (!wallpaper_file.isEmpty()) {
            screensaver_image = load_scaled_image(wallpaper_file, screen_size);
        }

        return;
    }

    // 5. Handle transparent mode
    QPixmap screenshot_pixmap;
    QImage wallpaper_image;

    bool glitch_enabled = settings.value(GLITCH_ENABLED, false).toBool();
    int glitch_iterations = qBound(2, settings.value(GLITCH_ITERATIONS, 5).toInt(), 10);
    int glitch_quality = qBound(10, settings.value(GLITCH_QUALITY, 80).toInt(), 100);

    if (display_mode & DISPLAY_MODE::Book) {
        // Take screenshot of the current screen if reading
        QRect geometry = current_view->geometry();
        screenshot_pixmap = screen->grabWindow(
            desktop_widget->winId(),
            geometry.left(),
            geometry.top(),
            geometry.width(),
            geometry.height()
        );

        if (glitch_enabled) {
            wallpaper_image = glitch_pixmap(screenshot_pixmap, glitch_iterations, glitch_quality);
        }
    } else if (display_mode & DISPLAY_MODE::Wallpaper and !wallpaper_file.isEmpty()) {
        wallpaper_image = load_scaled_image(wallpaper_file, screen_size);
    }

    // 6. Combine overlay & wallpaper into target image
    QPaintDevice *backing;
    if (is_overlay_wallpaper) {
        if (overlay_pixmap.isNull() || overlay_pixmap.size() != screen_size) {
            overlay_pixmap = QPixmap(screen_size);
        }
        overlay_pixmap.fill(Qt::transparent);
        backing = &overlay_pixmap;
    } else {
        if (screensaver_image.isNull() || screensaver_image.size() != screen_size) {
            screensaver_image = QImage(screen_size, QImage::Format_RGB32);
        }
        screensaver_image.fill(Qt::white);
        backing = &screensaver_image;
    }

    QPainter painter(backing); // this will copy the image data if it's referenced somewhere else
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // Draw wallpaper
    if (!wallpaper_image.isNull()) {
        nh_log("wallpaper_image %d %d", wallpaper_image.width(), wallpaper_image.height());
        painter.drawImage(0, 0, wallpaper_image);
    } else if (!screenshot_pixmap.isNull()) {
        if (screenshot_pixmap.size() != screen_size) {
            // Only scale if size mismatch
            painter.drawPixmap(0, 0, screenshot_pixmap.scaled(screen_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation));
        } else {
            painter.drawPixmap(0, 0, screenshot_pixmap);
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

    // 8. Done
    N3PowerWorkflowManager_handleSleep(self);
    // nh_log("Current view: %s", current_view_name.toStdString().c_str());
    // nh_dump_log();
}

void after_view_shown(bool allow_charging_overlay) {
    // Remove blank screensaver
    QFile file("/mnt/onboard/.kobo/screensaver/nickel-screensaver.png");
    file.remove();

    void *mwc = MainWindowController_sharedInstance();
    QWidget *current_view = MainWindowController_currentView(mwc);
    if (!current_view) {
        return;
    }

    // Avoid adding overlay to the wrong view when unlocking the device instantly
    // https://github.com/redphx/nickel-screensaver/issues/12
    QString current_view_name = current_view->objectName();
    if (!current_view_name.contains(QStringLiteral("DragonPowerView"))) {
        return;
    }

    // Check if cover mode
    if (is_overlay_wallpaper) {
        if (!overlay_pixmap.isNull()) {
            QLabel* overlay = new QLabel(current_view);
            overlay->setPixmap(overlay_pixmap);
            overlay->setGeometry(current_view->rect());
            if (current_view_name != QStringLiteral("FramedDragonPowerView")) {
                // Not reading Instapaper article
                overlay->lower();
            }
            overlay->show();
        }
    } else if (!screensaver_image.isNull()) {
        if (current_view_name == QStringLiteral("FramedDragonPowerView")) {
            // Instapaper
            QLabel* overlay = new QLabel(current_view);
            overlay->setPixmap(QPixmap::fromImage(screensaver_image));
            overlay->setGeometry(current_view->rect());
            overlay->show();
        } else if (FullScreenDragonPowerView_setImage) {
            FullScreenDragonPowerView_setImage(current_view, screensaver_image);
        }
    }

    bool charging_overlay_enabled = false;
    if (allow_charging_overlay && QDir("/mnt/onboard/.kobo/screensaver").exists()) {
        QSettings settings("/mnt/onboard/.adds/screensaver/_settings.ini", QSettings::IniFormat);
        charging_overlay_enabled = settings.value(BATTERY_ENABLED, true).toBool();
    }

    QWidget *charging_overlay = current_view->findChild<QWidget*>(
        QStringLiteral("nickelChargingOverlay"), Qt::FindDirectChildrenOnly);
    if (charging_overlay_enabled) {
        if (!charging_overlay) {
            charging_overlay = new ChargingOverlay(current_view);
        }
        charging_overlay->raise();
    } else if (!charging_overlay_enabled && charging_overlay) {
        charging_overlay->hide();
    }

    // BookCoverDragonPowerView_setInfoPanelVisible(current_view, true);
}

extern "C" __attribute__((visibility("default")))
void hook_N3PowerWorkflowManager_handleSleep(N3PowerWorkflowManager* self) {
    before_handle(self);

    N3PowerWorkflowManager_handleSleep(self);
}

extern "C" __attribute__((visibility("default")))
void hook_N3PowerWorkflowManager_powerOff(N3PowerWorkflowManager* self, bool low_battery) {
    before_handle(self);

    N3PowerWorkflowManager_powerOff(self, low_battery);
}

extern "C" __attribute__((visibility("default")))
void hook_N3PowerWorkflowManager_showSleepView(N3PowerWorkflowManager* self) {
    N3PowerWorkflowManager_showSleepView(self);

    after_view_shown(true);
}

extern "C" __attribute__((visibility("default")))
void hook_N3PowerWorkflowManager_showPowerOffView(N3PowerWorkflowManager* self) {
    N3PowerWorkflowManager_showPowerOffView(self);

    after_view_shown(false);
}
