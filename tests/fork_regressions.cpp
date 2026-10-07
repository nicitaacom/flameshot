// SPDX-License-Identifier: GPL-3.0-or-later
#include "config/generalconf.h"
#include "config/configwindow.h"
#include "config/shortcutswidget.h"
#include "core/flameshotdaemon.h"
#include "utils/confighandler.h"
#include "widgets/capture/capturewidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QClipboard>
#include <QSignalSpy>
#include <QGroupBox>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QProcess>
#include <dlfcn.h>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTemporaryDir>
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
    const auto sections = general.findChildren<QGroupBox*>();
    require(!sections.isEmpty() && sections.first()->title() == "Options",
            "Options appears first when General settings opens");
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
    general.resize(640, 600);
    general.show();
    QTest::qWait(20);
    const QRect checkboxBounds(show->mapTo(&general, QPoint()), show->size());
    require(general.rect().contains(checkboxBounds),
            "Show resolution is visible without scrolling when General opens");
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
    QLabel* quality = nullptr;
    QSpinBox* qualityValue = nullptr;
    for (auto* label : general.findChildren<QLabel*>()) {
        if (label->text() == "JPEG Quality") {
            quality = label;
        }
    }
    for (auto* spin : general.findChildren<QSpinBox*>()) {
        if (spin->maximum() == 100) {
            qualityValue = spin;
        }
    }
    require(quality && qualityValue &&
              quality->mapTo(&general, QPoint()).x() < qualityValue->mapTo(&general, QPoint()).x(),
            "JPEG Quality label precedes its value");
    ConfigWindow window;
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();
    window.resize(800, 700);
    QTest::qWait(20);
    require(window.height() == 700,
            "Settings window can resize without the conflicting half-screen cap");
    qInfo("PASS: visible resolution settings, section layout, resize and legacy compatibility");
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

static void saveLocations()
{
    ConfigHandler config;
    require(config.savePathLocationCount() == 3,
            "Three save locations are enabled by default");
    const auto defaults = config.buttons();
    for (int i = 1; i <= 10; ++i) {
        const auto type = static_cast<CaptureTool::Type>(
          CaptureTool::TYPE_SAVE_LOCATION_1 + i - 1);
        require(defaults.contains(type) == (i <= 3),
                "Only the original three save tools are visible by default");
    }
    GeneralConf general;
    QPushButton* add = nullptr;
    QGroupBox* locations = nullptr;
    for (auto* box : general.findChildren<QGroupBox*>()) {
        if (box->title().startsWith("Save Path Locations")) {
            locations = box;
        }
    }
    require(locations != nullptr, "Save locations section exists");
    for (auto* button : locations->findChildren<QPushButton*>()) {
        if (button->text() == "Add Location") {
            add = button;
        }
    }
    require(add != nullptr, "Add Location is available");
    require(locations->findChildren<QLineEdit*>().size() == 3,
            "General settings initially show three locations");
    for (int i = 4; i <= 10; ++i) {
        add->click();
    }
    require(config.savePathLocationCount() == 10 && !add->isEnabled() &&
              locations->findChildren<QLineEdit*>().size() == 10,
            "All ten locations can be added without exceeding the limit");
    QTemporaryDir destination;
    require(destination.isValid(), "Save test directory exists");
    config.setSaveAsFileExtension("png");
    config.setShowDesktopNotification(false);
    config.setShowHelp(false);
    config.setShowMagnifier(false);
    for (int i = 1; i <= 10; ++i) {
        const QString folder = destination.path() + QString("/location%1").arg(i);
        require(QDir().mkpath(folder), "Destination directory created");
        config.setSavePathLocation(i, folder);
        const QString name = QString("TYPE_SAVE_LOCATION_%1").arg(i);
        const QString key = QString("Alt+Shift+%1").arg(i % 10);
        require(config.setShortcut(name, key), "Every save location is bindable");
        CaptureRequest request(CaptureRequest::GRAPHICAL_MODE);
        request.setInitialSelection(QRect(200, 100, 300, 200));
        QPointer<CaptureWidget> capture = new CaptureWidget(request);
        capture->show();
        capture->activateWindow();
        capture->setFocus();
        QTest::qWait(50);
        bool bound = false;
        for (auto* shortcut : capture->findChildren<QShortcut*>()) {
            bound |= shortcut->key() == QKeySequence(key);
        }
        require(bound, "Save shortcut exists even when toolbar tool is hidden");
        QTest::keyClick(capture, static_cast<Qt::Key>(Qt::Key_0 + i % 10),
                        Qt::AltModifier | Qt::ShiftModifier);
        QTest::qWait(50);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(capture.isNull(), "Save shortcut finishes the capture");
        const QStringList files = QDir(folder).entryList({"*.png"}, QDir::Files);
        require(files.size() == 1, "Each shortcut saves into its own folder");
        require(!QImage(folder + "/" + files.first()).isNull(),
                "Saved file is a valid image");
    }
    general.updateComponents();
    for (int i = 1; i <= 10; ++i) {
        require(locations->findChildren<QLineEdit*>().at(i - 1)->text() ==
                  config.savePathLocation(i),
                "Each configured folder appears in General settings");
    }
    ShortcutsWidget shortcuts;
    auto* table = shortcuts.findChild<QTableWidget*>();
    require(table != nullptr, "Shortcuts table exists");
    for (int i = 1; i <= 10; ++i) {
        require(table->item(i - 1, 0)->text() == QString("Save to loc%1").arg(i),
                "All ten save actions are listed in Shortcuts");
        require(table->item(i - 1, 1)->text() == QString("Alt+Shift+%1").arg(i % 10),
                "Configured shortcuts appear in the table");
    }
    qInfo("PASS: three default locations, ten configurable folders and real shortcut saves");
}

static void doubleClicks()
{
    ConfigHandler config;
    config.setCopyOnDoubleClick(true);
    config.setSaveAfterCopy(false);
    config.setShowDesktopNotification(false);
    config.setDisabledTrayIcon(true);
    config.setCheckForUpdates(false);
    config.setAutoCloseIdleDaemon(false);
    FlameshotDaemon::start();
    QTemporaryDir destination;
    require(destination.isValid(), "Double-click test directory exists");
    config.setSavePath(destination.path());
    config.setSavePathFixed(true);
    CaptureRequest request(CaptureRequest::GRAPHICAL_MODE);
    request.setInitialSelection(QRect(200, 100, 300, 200));
    QPointer<CaptureWidget> capture = new CaptureWidget(request);
    capture->show();
    capture->activateWindow();
    QTest::qWait(50);
    QApplication::clipboard()->setText("clipboard sentinel");
    QTest::mouseDClick(capture, Qt::RightButton, Qt::NoModifier, QPoint(350, 200));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    require(capture && capture->isVisible(), "Right double-click keeps capture open");
    require(QApplication::clipboard()->text() == "clipboard sentinel",
            "Right double-click leaves the clipboard alone");
    config.setCopyOnDoubleClick(false);
    QTest::mouseDClick(capture, Qt::LeftButton, Qt::NoModifier, QPoint(350, 200));
    require(QApplication::clipboard()->text() == "clipboard sentinel",
            "Disabled left double-click leaves the clipboard alone");
    config.setCopyOnDoubleClick(true);
    QTest::mouseDClick(capture, Qt::LeftButton, Qt::NoModifier, QPoint(350, 200));
    QTest::qWait(50);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    require(capture && capture->isVisible(), "Left double-click keeps capture open");
    const QPixmap copied = QApplication::clipboard()->pixmap();
    require(!copied.isNull() && copied.size() == capture->pixmap().size(),
            "Left double-click copies the selected capture");
    require(QDir(destination.path()).entryList(QDir::Files).isEmpty(),
            "Left double-click does not save when Save-after-copy is disabled");
    config.setSaveAfterCopy(true);
    QTest::mouseDClick(capture, Qt::LeftButton, Qt::NoModifier, QPoint(350, 200));
    QTest::qWait(50);
    require(capture && capture->isVisible() &&
              QDir(destination.path()).entryList({"*.png"}, QDir::Files).size() == 1,
            "Left double-click preserves the configured Save-after-copy behavior");
    capture->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    qInfo("PASS: both double-clicks stay open; left copies and honors Save-after-copy");
}

static void checkImportedProfile()
{
    ConfigHandler config;
    require(config.drawThickness() == 4 && config.jpegQuality() == 73 &&
              config.undoLimit() == 42 && !config.insecurePixelate(),
            "Imported drawing and save preferences remain active");
    require(!config.showSelectionGeometryEnabled() &&
              config.showSelectionGeometry() == GeneralConf::xywh_top_left &&
              config.value("showSelectionGeometryHideTime").toInt() == 2700,
            "Imported resolution preferences remain active");
    require(config.savePathLocationCount() == 10 &&
              config.savePathLocation(10).endsWith("/imported-location10") &&
              config.shortcut("TYPE_SAVE_LOCATION_10") == "Ctrl+Alt+F10",
            "Imported tenth save location and shortcut remain active");
    require(!config.hasError(), "Imported profile passes configuration validation");
}

static void importedProfilePersistence()
{
    using SetTime = void (*)(int64_t);
    auto setTime = reinterpret_cast<SetTime>(dlsym(RTLD_DEFAULT, "setForkTestTime"));
    require(setTime != nullptr, "Process-local test clock is available");
    const QDate today(2026, 10, 7);
    setTime(QDateTime(today, QTime(23, 59, 50)).toSecsSinceEpoch());
    require(QDateTime::currentDateTime().date() == today &&
              QTime::currentTime().hour() == 23,
            "Test begins just before local midnight");
    ConfigHandler config;
    ConfigHandler liveSettings;
    liveSettings.setDrawThickness(19); // Pending write from before the import.
    GeneralConf general;
    QObject::connect(ConfigHandler::getInstance(), &ConfigHandler::fileChanged,
                     &general, &GeneralConf::updateComponents);
    QSignalSpy refreshed(ConfigHandler::getInstance(), &ConfigHandler::fileChanged);
    QTemporaryDir fixture;
    require(fixture.isValid(), "Import fixture directory exists");
    const QString fileName = fixture.path() + "/profile.ini";
    {
        QSettings profile(fileName, QSettings::IniFormat);
        profile.setValue("drawThickness", 4);
        profile.setValue("jpegQuality", 73);
        profile.setValue("undoLimit", 42);
        profile.setValue("insecurePixelate", false);
        profile.setValue("disabledTrayIcon", true);
        profile.setValue("checkForUpdates", false);
        profile.setValue("showSelectionGeometryEnabled", false);
        profile.setValue("showSelectionGeometry", GeneralConf::xywh_top_left);
        profile.setValue("showSelectionGeometryHideTime", 2700);
        profile.setValue("savePathLocationCount", 10);
        for (int i = 1; i <= 10; ++i) {
            profile.setValue(QString("savePathLocation%1").arg(i),
                             fixture.path() + QString("/imported-location%1").arg(i));
            profile.setValue(QString("Shortcuts/TYPE_SAVE_LOCATION_%1").arg(i),
                             QString("Ctrl+Alt+F%1").arg(i));
        }
        profile.sync();
        require(profile.status() == QSettings::NoError, "Profile fixture written");
    }
    require(ConfigHandler::getInstance()->importConfiguration(fileName),
            "Profile import succeeds with existing settings objects");
    require(!refreshed.isEmpty(), "Successful import refreshes the UI immediately");
    checkImportedProfile();
    bool showChecked = true;
    for (auto* check : general.findChildren<QCheckBox*>()) {
        if (check->text() == "Show resolution") {
            showChecked = check->isChecked();
        }
    }
    require(!showChecked, "Imported resolution state appears in the open settings");
    const auto spins = general.findChildren<QSpinBox*>();
    bool qualityRefreshed = false, timeoutRefreshed = false;
    for (auto* spin : spins) {
        qualityRefreshed |= spin->maximum() == 100 && spin->value() == 73;
        timeoutRefreshed |= spin->value() == 2700;
    }
    require(qualityRefreshed && timeoutRefreshed,
            "Imported JPEG quality and resolution timeout refresh immediately");
    setTime(QDateTime(today.addDays(1), QTime(0, 0, 5)).toSecsSinceEpoch());
    require(QDateTime::currentDateTime().date() == today.addDays(1) &&
              QTime::currentTime().hour() == 0,
            "Wall clock crosses local midnight");
    QTest::qWait(150);
    liveSettings.setShowHelp(false);
    QSettings persisted(config.configFilePath(), QSettings::IniFormat);
    persisted.sync();
    general.updateComponents();
    checkImportedProfile();
    require(persisted.value("drawThickness").toInt() == 4 &&
              persisted.value("savePathLocation10").toString().endsWith("/imported-location10"),
            "Pending settings writes do not overwrite the import after midnight");
    QProcess restarted;
    restarted.start(QCoreApplication::applicationFilePath(), {"--check-import"});
    require(restarted.waitForFinished(10000) && restarted.exitCode() == 0 &&
              restarted.exitStatus() == QProcess::NormalExit,
            "Imported profile remains active in a fresh process");
    const QString invalid = fixture.path() + "/invalid.ini";
    {
        QSettings profile(invalid, QSettings::IniFormat);
        profile.setValue("drawThickness", -1);
    }
    require(!config.importConfiguration(invalid), "Invalid profile is rejected");
    checkImportedProfile();
    const QString empty = fixture.path() + "/empty.ini";
    QFile emptyFile(empty);
    require(emptyFile.open(QIODevice::WriteOnly), "Empty fixture created");
    emptyFile.close();
    require(!config.importConfiguration(empty), "Empty profile cannot reset settings");
    require(!config.importConfiguration(fixture.path() + "/missing.ini"),
            "Missing profile cannot reset settings");
    checkImportedProfile();
    require(config.importConfiguration(config.configFilePath()),
            "Importing the active profile is safe");
    checkImportedProfile();
    qInfo("PASS: import refresh, stale writes, midnight, process restart and invalid import rollback");
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("flameshot");
    app.setApplicationName("flameshot");
    app.setQuitOnLastWindowClosed(false);
    qRegisterMetaType<QList<int>>();
    require(ConfigHandler().configFilePath().startsWith(qEnvironmentVariable("XDG_CONFIG_HOME") + "/"),
            "Tests use disposable settings");
    if (app.arguments().contains("--check-import")) {
        checkImportedProfile();
        return 0;
    }
    resolutionSettings();
    resolutionOverlay();
    saveLocations();
    doubleClicks();
    importedProfilePersistence();
    return 0;
}
