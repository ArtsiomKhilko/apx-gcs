﻿/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "VehicleMission.h"
#include "MissionListModel.h"
#include "MissionTools.h"
#include "MissionStorage.h"
#include "MissionShare.h"
#include "LookupMissions.h"
#include "MissionGroup.h"
#include "MissionField.h"
#include "Waypoint.h"
#include "Runway.h"
#include "Taxiway.h"
#include "Poi.h"

#include <Vehicles/Vehicles.h>
#include <Protocols/ProtocolMission.h>
#include <QQmlEngine>
#include <ApxApp.h>
//=============================================================================
VehicleMission::VehicleMission(Vehicle *parent)
    : Fact(parent, "mission", "Mission", tr("Vehicle mission"), Group)
    , vehicle(parent)
    , blockSizeUpdate(false)
    , m_startHeading(0)
    , m_startLength(0)
    , m_missionSize(0)
    , m_empty(true)
    , m_synced(false)
    , m_saved(false)
{
    setIcon("ship-wheel");

    storage = new MissionStorage(this);
    connect(storage, &MissionStorage::loaded, this, [=]() {
        if (!empty())
            emit missionAvailable();
    });

    f_title = new MissionField(this, "missionTitle", tr("Title"), tr("Mission title"), Text);

    connect(f_title, &Fact::textChanged, this, &VehicleMission::updateStatus);
    connect(this, &VehicleMission::missionSizeChanged, this, &VehicleMission::updateStatus);

    //groups of items
    f_runways = new Runways(this, "runways", tr("Runways"), tr("Takeoff and Landing"));
    f_waypoints = new Waypoints(this, "waypoints", tr("Waypoints"), "");
    f_taxiways = new Taxiways(this, "taxiways", tr("Taxiways"), "");
    f_pois = new Pois(this, "points", tr("Points"), tr("Points of Interest"));

    foreach (MissionGroup *group, groups) {
        connect(group, &Fact::sizeChanged, this, &VehicleMission::updateSize, Qt::QueuedConnection);
    }

    //actions
    f_request = new FactAction(this,
                               "request",
                               tr("Request"),
                               tr("Download from vehicle"),
                               "download");

    f_upload = new FactAction(this,
                              "upload",
                              tr("Upload"),
                              tr("Upload to vehicle"),
                              "upload",
                              FactAction::ActionApply);
    connect(f_upload, &FactAction::triggered, this, &VehicleMission::uploadMission);

    f_clear = new FactAction(this,
                             "clear",
                             tr("Clear"),
                             tr("Clear mission"),
                             "",
                             FactAction::ActionRemove);
    f_clear->setFlag(FactAction::ActionHideTitle);
    connect(f_clear, &FactAction::triggered, this, &VehicleMission::clearMission);

    //tools actions
    f_tools = new MissionTools(this, nullptr);
    f_tools->setParent(this);
    new FactAction(this, f_tools, FactAction::ActionHideTitle);

    f_lookup = new LookupMissions(this, nullptr);
    f_lookup->setParent(this);
    new FactAction(f_tools, f_lookup);

    f_save = new FactAction(f_tools,
                            "save",
                            tr("Save mission"),
                            tr("Commit to database"),
                            "content-save",
                            FactAction::ActionApply);
    connect(f_save, &FactAction::triggered, storage, &MissionStorage::saveMission);

    f_share = new MissionShare(this, nullptr);
    f_share->setParent(this);
    new FactAction(f_tools, f_share);

    //ApxApp::jsync(f_tools);

    foreach (FactAction *a, actions) {
        //a->setFlag(FactAction::ActionHideTitle);
        connect(a, &FactAction::enabledChanged, this, &VehicleMission::actionsUpdated);
    }

    //internal
    m_listModel = new MissionListModel(this);

    connect(this, &VehicleMission::emptyChanged, this, &VehicleMission::updateActions);
    connect(this, &VehicleMission::savedChanged, this, &VehicleMission::updateActions);
    connect(this, &VehicleMission::syncedChanged, this, &VehicleMission::updateActions);

    connect(this, &VehicleMission::startPointChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startHeadingChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startLengthChanged, this, &VehicleMission::updateStartPath);

    //sync and saved status behavior
    connect(this, &Fact::modifiedChanged, this, [=]() {
        if (modified()) {
            setSynced(false);
            setSaved(false);
        }
    });
    connect(this, &VehicleMission::emptyChanged, this, [=]() {
        if (empty()) {
            setSynced(false);
            setSaved(true);
        }
    });
    connect(this, &VehicleMission::missionAvailable, this, [=]() {
        setSynced(true);
        setSaved(false);
    });
    connect(storage, &MissionStorage::loaded, this, [=]() {
        setSynced(false);
        setSaved(true);
    });
    connect(storage, &MissionStorage::saved, this, [=]() {
        setSaved(true);
        setModified(false, true);
    });

    //protocols
    if (vehicle->protocol) {
        connect(vehicle->protocol->mission,
                &ProtocolMission::missionDataReceived,
                this,
                &VehicleMission::missionDataReceived);
        connect(vehicle->protocol->mission,
                &ProtocolMission::missionDataError,
                this,
                &VehicleMission::missionDataError);
        connect(this,
                &VehicleMission::missionDataUpload,
                vehicle->protocol->mission,
                &ProtocolMission::missionDataUpload);
        connect(f_request,
                &FactAction::triggered,
                vehicle->protocol->mission,
                &ProtocolMission::downloadMission);
    }

    //reset and update
    clearMission();
    updateActions();
    updateStatus();

    if (!vehicle->isLocal()) {
        //f_request->trigger();
        QTimer::singleShot(2000, f_request, &FactAction::trigger);
    }

    qmlRegisterUncreatableType<VehicleMission>("APX.Mission", 1, 0, "Mission", "Reference only");
    qmlRegisterUncreatableType<MissionItem>("APX.Mission", 1, 0, "MissionItem", "Reference only");
    qmlRegisterUncreatableType<Waypoint>("APX.Mission", 1, 0, "Waypoint", "Reference only");
    qmlRegisterUncreatableType<Runway>("APX.Mission", 1, 0, "Runway", "Reference only");
    qmlRegisterUncreatableType<MissionListModel>("APX.Mission",
                                                 1,
                                                 0,
                                                 "MissionListModel",
                                                 "Reference only");

    ApxApp::jsync(this);
}
//=============================================================================
void VehicleMission::updateActions()
{
    bool bEmpty = empty();
    f_upload->setEnabled(!bEmpty);
    f_clear->setEnabled(!bEmpty);
    f_save->setEnabled((!bEmpty) && (!saved()));
    f_share->f_export->setEnabled(!bEmpty);
}
void VehicleMission::updateSize()
{
    if (blockSizeUpdate)
        return;
    int cnt = 0;
    foreach (MissionGroup *group, groups) {
        cnt += group->size();
    }
    setMissionSize(cnt);
}
void VehicleMission::updateStatus()
{
    QString s = f_title->text();
    int sz = missionSize();
    if (sz <= 0 && s.isEmpty())
        s = title();
    else if (s.isEmpty())
        s = QString("(%1)").arg(tr("No title"));
    if (sz > 0)
        s.append(QString(" [%1]").arg(sz));
    setStatus(s.simplified());
}
//=============================================================================
void VehicleMission::updateStartPath()
{
    if (f_waypoints->size() <= 0)
        return;
    static_cast<Waypoint *>(f_waypoints->child(0))->updatePath();
}
//=============================================================================
QGeoRectangle VehicleMission::boundingGeoRectangle() const
{
    QGeoRectangle r;
    foreach (MissionGroup *group, groups) {
        for (int i = 0; i < group->size(); ++i) {
            QGeoRectangle re = static_cast<MissionItem *>(group->child(i))->boundingGeoRectangle();
            r = r.isValid() ? r.united(re) : re;
        }
    }
    r.setWidth(r.width() * 1.2);
    r.setHeight(r.height() * 1.2);
    return r;
}
//=============================================================================
//=============================================================================
void VehicleMission::clearMission()
{
    f_title->setValue("");
    setSite("");
    setMissionSize(0);
    blockSizeUpdate = true;
    foreach (MissionGroup *group, groups) {
        group->f_clear->trigger();
    }
    blockSizeUpdate = false;
    setModified(false, true);

    ApxApp::jsync(this);
}
//=============================================================================
void VehicleMission::backup()
{
    foreach (MissionGroup *group, groups) {
        group->backup();
    }
    f_title->backup();
    setModified(false, true);
}
void VehicleMission::restore()
{
    foreach (MissionGroup *group, groups) {
        group->restore();
    }
    f_title->restore();
    setModified(false, true);
}
void VehicleMission::hashData(QCryptographicHash *h) const
{
    foreach (MissionGroup *group, groups) {
        group->hashData(h);
    }
}
//=============================================================================
void VehicleMission::test(int n)
{
    if (f_waypoints->size() <= 0)
        return;
    Waypoint *w = f_waypoints->child<Waypoint>(f_waypoints->size() - 1);
    QGeoCoordinate p(w->f_latitude->value().toDouble(), w->f_longitude->value().toDouble());
    double hdg = 360.0 * qrand() / RAND_MAX;
    for (int i = 0; i < n; ++i) {
        hdg += 200.0 * qrand() / RAND_MAX - 100.0;
        p = p.atDistanceAndAzimuth(100 + 10000.0 * qrand() / RAND_MAX, hdg);
        f_waypoints->addObject(p);
    }
}
//=============================================================================
//=============================================================================
void VehicleMission::missionDataReceived(DictMission::Mission d)
{
    clearMission();
    storage->loadFromDict(d);
    if (empty()) {
        apxMsgW() << tr("Empty mission received from vehicle");
    } else {
        emit missionAvailable();
        emit missionDownloaded();
        f_save->trigger();
        apxMsg() << tr("Mission received from vehicle") << size();
    }
    setModified(false, true);
}
void VehicleMission::missionDataError()
{
    apxMsgW() << tr("Error in mission data from vehicle");
    clearMission();
}
//=============================================================================
void VehicleMission::uploadMission()
{
    emit missionDataUpload(storage->saveToDict());
    emit missionUploaded();
    f_save->trigger();
}
//=============================================================================
//=============================================================================
QGeoCoordinate VehicleMission::startPoint() const
{
    return m_startPoint;
}
void VehicleMission::setStartPoint(const QGeoCoordinate &v)
{
    if (m_startPoint == v)
        return;
    m_startPoint = v;
    emit startPointChanged();
}
double VehicleMission::startHeading() const
{
    return m_startHeading;
}
void VehicleMission::setStartHeading(const double &v)
{
    if (m_startHeading == v)
        return;
    m_startHeading = v;
    emit startHeadingChanged();
}
double VehicleMission::startLength() const
{
    return m_startLength;
}
void VehicleMission::setStartLength(const double &v)
{
    if (m_startLength == v)
        return;
    m_startLength = v;
    emit startLengthChanged();
}
MissionListModel *VehicleMission::listModel() const
{
    return m_listModel;
}
int VehicleMission::missionSize() const
{
    return m_missionSize;
}
void VehicleMission::setMissionSize(const int v)
{
    if (m_missionSize == v)
        return;
    m_missionSize = v;
    emit missionSizeChanged();
    setEmpty(v <= 0);
}
bool VehicleMission::empty() const
{
    return m_empty;
}
void VehicleMission::setEmpty(const bool v)
{
    if (m_empty == v)
        return;
    m_empty = v;
    emit emptyChanged();
}
QGeoCoordinate VehicleMission::coordinate() const
{
    return m_coordinate;
}
void VehicleMission::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
QString VehicleMission::site() const
{
    return m_site;
}
void VehicleMission::setSite(const QString &v)
{
    if (m_site == v)
        return;
    m_site = v;
    emit siteChanged();
}
bool VehicleMission::synced() const
{
    return m_synced;
}
void VehicleMission::setSynced(const bool v)
{
    if (m_synced == v)
        return;
    m_synced = v;
    emit syncedChanged();
}
bool VehicleMission::saved() const
{
    return m_saved;
}
void VehicleMission::setSaved(const bool v)
{
    if (m_saved == v)
        return;
    m_saved = v;
    emit savedChanged();
}
//=============================================================================
//=============================================================================