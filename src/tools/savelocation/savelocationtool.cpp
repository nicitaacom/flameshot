// SPDX-License-Identifier: GPL-3.0-or-later

#include "savelocationtool.h"
#include "utils/confighandler.h"

#include <QPainter>

SaveLocationTool::SaveLocationTool(int location, QObject* parent)
  : AbstractActionTool(parent)
  , m_location(location)
{}

bool SaveLocationTool::closeOnButtonPressed() const
{
    return true;
}

QIcon SaveLocationTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "content-save.svg");
}

QString SaveLocationTool::name() const
{
    return tr("Save to loc%1").arg(m_location);
}

QString SaveLocationTool::description() const
{
    return tr("Save to loc%1").arg(m_location);
}

CaptureTool::Type SaveLocationTool::type() const
{
    return static_cast<CaptureTool::Type>(
      CaptureTool::TYPE_SAVE_LOCATION_1 + m_location - 1);
}

CaptureTool* SaveLocationTool::copy(QObject* parent)
{
    return new SaveLocationTool(m_location, parent);
}

void SaveLocationTool::pressed(CaptureContext& context)
{
    const QString path = ConfigHandler().savePathLocation(m_location);

    emit requestAction(REQ_CLEAR_SELECTION);
    // An empty path falls back to the normal save-as dialog, same as
    // pressing Save with no configured location.
    context.request.addSaveTask(path);
    emit requestAction(REQ_CAPTURE_DONE_OK);
    emit requestAction(REQ_CLOSE_GUI);
}
