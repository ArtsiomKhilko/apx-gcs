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
#include <ApxLog.h>
#include <ApxApp.h>
#include "Vehicles.h"
#include "VehicleSelect.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "VehicleWarnings.h"
#include <Database/TelemetryDB.h>
#include <QQmlEngine>
//=============================================================================
APX_LOGGING_CATEGORY(VehiclesLog, "core.vehicles")
Vehicles *Vehicles::_instance = nullptr;
Vehicles::Vehicles(Fact *parent, ProtocolVehicles *protocol)
    : Fact(parent, "vehicles", tr("Vehicles"), tr("Discovered vehicles"), Section)
    , protocol(protocol)
{
    _instance = this;

    qmlRegisterUncreatableType<Vehicles>("APX.Vehicles", 1, 0, "Vehicles", "Reference only");
    qmlRegisterUncreatableType<Vehicle>("APX.Vehicles", 1, 0, "Vehicle", "Reference only");
    qmlRegisterUncreatableType<VehicleWarnings>("APX.Vehicles",
                                                1,
                                                0,
                                                "VehicleWarnings",
                                                "Reference only");

    VehicleSelect *f_select = new VehicleSelect(this,
                                                "select",
                                                tr("Select vehicle"),
                                                tr("Change the active vehicle"));
    f_select->setIcon("select");
    f_select->setSection(title());
    connect(f_select, &VehicleSelect::vehicleSelected, this, &Vehicles::selectVehicle);

    f_list = new Fact(this, "list", tr("Vehicles"), tr("Identified vehicles"), Group | Const);
    f_list->setIcon("airplane");

    f_local = new Vehicle(this,
                          "LOCAL",
                          0,
                          QByteArray().append((char) 0).append((char) 0),
                          Vehicle::LOCAL,
                          protocol->local);

    f_replay = new Vehicle(this,
                           "REPLAY",
                           0,
                           QByteArray().append((char) 0).append((char) 1),
                           Vehicle::REPLAY,
                           nullptr);

    f_select->addVehicle(f_local);
    f_select->addVehicle(f_replay);

    //JS register mandala
    ApxApp *app = ApxApp::instance();
    ApxApp::jsync(this);
    foreach (QString key, f_local->f_mandala->constants.keys())
        app->engine()->globalObject().setProperty(key,
                                                  app->engine()->toScriptValue(
                                                      f_local->f_mandala->constants.value(key)));
    foreach (VehicleMandalaFact *f, f_local->f_mandala->allFacts) {
        app->jsexec(QString("this.__defineGetter__('%1', function(){ return "
                            "apx.vehicles.current.mandala.%1.value; });")
                        .arg(f->name()));
        app->jsexec(QString("this.__defineSetter__('%1', function(v){ "
                            "apx.vehicles.current.mandala.%1.value=v; });")
                        .arg(f->name()));
    }

    //Database register fields
    DatabaseRequest::Records recFields;
    recFields.names << "name"
                    << "title"
                    << "descr"
                    << "units"
                    << "opts"
                    << "sect"
                    << "id";
    foreach (VehicleMandalaFact *f, f_local->f_mandala->allFacts) {
        recFields.values.append(QVariantList()
                                << f->name() << f->title() << f->descr() << f->units()
                                << f->enumStrings().join(',') << QVariant() << f->id());
    }
    (new DBReqTelemetryUpdateFields(recFields))->exec();

    selectVehicle(f_local);

    f_local->dbSaveVehicleInfo();

    //connect protocols
    connect(protocol, &ProtocolVehicles::vehicleIdentified, this, &Vehicles::vehicleIdentified);
}
//=============================================================================
void Vehicles::vehicleIdentified(ProtocolVehicle *protocol)
{
    Vehicle *v = new Vehicle(this,
                             protocol->ident.callsign,
                             protocol->squawk,
                             protocol->ident.uid,
                             (Vehicle::VehicleClass) protocol->ident.vclass,
                             protocol);

    apxMsg() << tr("Vehicle identified").append(":") << v->vehicleClassText()
             << "'" + v->callsign() + "'"
             << "(" + v->squawkText() + ")";
    emit vehicleRegistered(v);

    v->dbSaveVehicleInfo();

    if (f_list->size() == 1)
        selectVehicle(v);
}
void Vehicles::identAssigned(ProtocolVehicle *v, const ProtocolVehicles::IdentData &ident)
{
    apxMsg() << tr("Assigning squawk to") << ident.callsign
             << "(" + QString::number(v->squawk, 16).toUpper() + "->"
                    + QString::number(v->squawk, 16).toUpper() + ")";
}
//=============================================================================
void Vehicles::selectVehicle(Vehicle *v)
{
    if (m_current == v)
        return;
    if (!v)
        return;
    //v->f_recorder->recordEvent("info",QString("%1: %2 '%3' (%4)").arg("Vehicle selected").arg(v->f_vclass->text()).arg(v->f_callsign->text()).arg(v->f_squawk->text()));
    apxMsg() << tr("Vehicle selected").append(":") << v->vehicleClassText()
             << "'" + v->callsign() + "'"
             << "(" + v->squawkText() + ")";
    m_current = v;
    //update JSengine
    //for QML
    ApxApp::instance()->engine()->rootContext()->setContextProperty("m", v->f_mandala);
    //for console
    //e->globalObject().setProperty("m",e->newQObject(v->f_mandala));

    //current vehicle signals wrappers
    foreach (QMetaObject::Connection c, currentVehicleConnections)
        disconnect(c);
    currentVehicleConnections.clear();
    currentVehicleConnections.append(connect(v,
                                             &Vehicle::downstreamDataReceived,
                                             this,
                                             &Vehicles::currentDownstreamDataReceived));
    currentVehicleConnections.append(
        connect(v, &Vehicle::serialDataReceived, this, &Vehicles::currentSerialDataReceived));

    emit currentChanged();
    emit vehicleSelected(v);
}
//=============================================================================
Vehicle *Vehicles::current(void) const
{
    return m_current;
}
//=============================================================================
//=============================================================================
void Vehicles::selectPrev()
{
    int i = f_list->indexOfChild(m_current);
    if (i < 0)
        i = 0;
    else if (i == 0)
        i = f_list->size() - 1;
    else if (i >= (f_list->size() - 1))
        i = 0;
    else
        i--;
    selectVehicle(qobject_cast<Vehicle *>(f_list->child(i)));
}
void Vehicles::selectNext()
{
    int i = f_list->indexOfChild(m_current);
    if (i < 0)
        i = 0;
    else if (i >= (f_list->size() - 1))
        i = 0;
    else
        i++;
    selectVehicle(qobject_cast<Vehicle *>(f_list->child(i)));
}
//=============================================================================