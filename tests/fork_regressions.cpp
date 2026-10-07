// SPDX-License-Identifier: GPL-3.0-or-later
#include "config/generalconf.h"
#include "utils/confighandler.h"
#include "widgets/capture/capturewidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QTest>

static void require(bool condition, const char* message)
{
    if (!condition) {
        qFatal("FAIL: %s", message);
    }
}

static void resolutionSettings()
{
    ConfigHandler config;
    GeneralConf general;
    QGroupBox* options = nullptr;
    for (auto* box : general.findChildren<QGroupBox*>()) {
        if (box->title() == "Options") {
            options = box;
        }
    }
    require(options != nullptr, "Options section exists");
    const auto checks = options->findChildren<QCheckBox*>();
    require(!checks.isEmpty() && checks.first()->text() == "Show resolution",
            "Show resolution is the first Options checkbox");
    auto* show = checks.first();
    bool hasInfo = false;
    for (auto* label : options->findChildren<QLabel*>()) {
        hasInfo |= label->toolTip().contains("646x319+740+256") &&
                   !label->pixmap().isNull();
    }
    require(hasInfo, "Resolution has an information icon and helpful tooltip");
    QComboBox* location = nullptr;
    for (auto* combo : options->findChildren<QComboBox*>()) {
        if (combo->findData(GeneralConf::xywh_bottom_right) >= 0) {
            location = combo;
        }
    }
    require(location != nullptr, "Resolution position is configurable");
    location->setCurrentIndex(location->findData(GeneralConf::xywh_top_left));
    show->click();
    require(!config.showSelectionGeometryEnabled() && !location->isEnabled(),
            "Checkbox disables overlay and its location control");
    show->click();
    require(config.showSelectionGeometryEnabled() &&
              config.showSelectionGeometry() == GeneralConf::xywh_top_left,
            "Re-enabling preserves the selected position");
    config.setShowSelectionGeometry(GeneralConf::xywh_none);
    general.updateComponents();
    require(!show->isChecked(), "Legacy None setting disables the checkbox");
    show->click();
    require(config.showSelectionGeometryEnabled() &&
              config.showSelectionGeometry() == GeneralConf::xywh_bottom_right,
            "Enabling a legacy None setting selects a visible position");
    qInfo("PASS: resolution settings and legacy compatibility");
}

static void resolutionOverlay()
{
    ConfigHandler config;
    config.setShowHelp(false);
    config.setShowMagnifier(false);
    config.setShowSelectionGeometry(GeneralConf::xywh_center);
    config.setValue("showSelectionGeometryHideTime", 0);
    config.setShowSelectionGeometryEnabled(true);
    CaptureRequest request(CaptureRequest::GRAPHICAL_MODE);
    request.setInitialSelection(QRect(200, 100, 300, 200));
    auto* capture = new CaptureWidget(request);
    capture->show();
    QTest::qWait(50);
    capture->showxywh();
    const QImage shown = capture->grab().toImage();
    config.setShowSelectionGeometryEnabled(false);
    const QImage hidden = capture->grab().toImage();
    require(shown != hidden, "Checkbox changes the actual capture overlay");
    config.setShowSelectionGeometryEnabled(true);
    require(capture->grab().toImage() == shown,
            "Overlay reappears when re-enabled");
    capture->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    qInfo("PASS: resolution overlay renders only when enabled");
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("flameshot");
    app.setApplicationName("flameshot");
    app.setQuitOnLastWindowClosed(false);
    qRegisterMetaType<QList<int>>();
    resolutionSettings();
    resolutionOverlay();
    return 0;
}
