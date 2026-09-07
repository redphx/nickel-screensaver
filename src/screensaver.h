#pragma once

#include <QLabel>
#include <QTimer>

class QEvent;

class ChargingOverlay : public QLabel {
    Q_OBJECT

public:
    explicit ChargingOverlay(QWidget *parent);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refresh();
    void usb_unplugged();

private:
    void update_geometry();

    QString battery_path;
    QTimer refresh_timer;
    QObject *hardware;
    uint (*charging_state)(QObject *self);
};

#ifndef NICKEL_SCREENSAVER_DELETE_FILE
	#define NICKEL_SCREENSAVER_DELETE_FILE "/mnt/onboard/.adds/screensaver/uninstall.txt"
#endif
