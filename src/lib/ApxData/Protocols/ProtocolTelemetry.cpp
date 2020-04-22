/*
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
#include "ProtocolTelemetry.h"
#include "ProtocolVehicle.h"

#include <App/App.h>
#include <Mandala/Mandala.h>

#include <Xbus/telemetry/TelemetryValuePack.h>

ProtocolTelemetry::ProtocolTelemetry(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "telemetry")
    , vehicle(vehicle)
{
    setIcon("sitemap");
    setTitle(tr("Telemetry"));
    setDescr(tr("Downlink stream decoder"));

    connect(this, &Fact::enabledChanged, this, &ProtocolTelemetry::updateStatus);
}

void ProtocolTelemetry::updateStatus()
{
    if (!enabled()) {
        setValue("RESYNC");
        return;
    }
    setValue(
        QString("%1 slots, %2 Hz").arg(decoder.slots_cnt()).arg(static_cast<int>(decoder.rate_hz())));
}

void ProtocolTelemetry::downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    trace_downlink(stream.payload());

    if (pid.uid != mandala::cmd::env::telemetry::data::uid) {
        // regular mandala value
        if (pid.uid >= mandala::cmd::env::uid)
            return;
        if (stream.available() <= mandala::spec_s::psize())
            return;

        mandala::spec_s spec;
        spec.read(&stream);

        TelemetryValues values = unpack(pid, spec, stream);
        if (values.isEmpty()) {
            qWarning() << "unpack data values error";
            return;
        }
        vehicle->updateStreamType(ProtocolVehicle::DATA);
        emit valuesData(values);
        return;
    }

    if (stream.available() < xbus::telemetry::stream_s::psize()) {
        qWarning() << stream.available();
        return;
    }

    vehicle->updateStreamType(ProtocolVehicle::TELEMETRY);

    bool upd = decoder.decode(pid, stream);
    bool valid = decoder.valid();

    if (enabled() && !valid) {
        qWarning() << "stream error";
    }

    setEnabled(valid);

    if (valid && _rate_s != decoder.rate_hz()) {
        _rate_s = decoder.rate_hz();
        updateStatus();
    }

    //qDebug() << valid << upd;

    if (!valid)
        return;

    if (!upd)
        return;

    // collect updated values
    TelemetryValues values;

    for (size_t i = 0; i < decoder.slots_cnt(); ++i) {
        auto &d = decoder.dec_slots().data[i];
        if (!d.upd)
            continue;
        d.upd = false;
        auto const &f = decoder.dec_slots().fields[i];
        TelemetryValue value;
        value.pid = f.pid;
        value.value = raw_value(&d.value, d.type);
        values.append(value);
    }

    emit telemetryData(values);
}

QVariant ProtocolTelemetry::raw_value(const void *src, mandala::type_id_e type)
{
    switch (type) {
    default:
        return QVariant();
    case mandala::type_real:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::real_t>(src, type));
    case mandala::type_dword:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::dword_t>(src, type));
    case mandala::type_word:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::word_t>(src, type));
    case mandala::type_byte:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::byte_t>(src, type));
    case mandala::type_option:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::option_t>(src, type));
    }
}

ProtocolTelemetry::TelemetryValues ProtocolTelemetry::unpack(const xbus::pid_s &pid,
                                                             const mandala::spec_s &spec,
                                                             ProtocolStreamReader &stream)
{
    TelemetryValues values;

    if (pid.pri > 0) {
        qWarning() << "pri:" << pid.pri << Mandala::meta(pid.uid).path;
    }

    if (spec.type >= mandala::type_bundle) {
        int vcnt = 0;
        switch (spec.type) {
        default:
            break;
        case mandala::type_vec2:
            vcnt = 2;
            break;
        case mandala::type_vec3:
            vcnt = 3;
            break;
        }
        mandala::spec_s vspec{};
        xbus::pid_s vpid{pid};
        for (int i = 0; i < vcnt; ++i) {
            vspec.type = Mandala::meta(vpid.uid).type_id;
            TelemetryValues vlist = unpack(vpid, vspec, stream);
            if (vlist.isEmpty())
                break;
            values.append(vlist);
            vpid.uid++;
        }
        if (values.size() != vcnt)
            values.clear();
        return values;
    }

    TelemetryValue value;
    value.pid = pid;
    //value.value=mandala::read<QVariant>(spec.type,stream);

    //qDebug() << Mandala::meta(pid.uid).name << stream->dump_payload();
    switch (spec.type) {
    default:
        break;
    case mandala::type_real:
        value.value = unpack<mandala::real_t>(stream);
        values.append(value);
        break;
    case mandala::type_dword:
        value.value = unpack<mandala::dword_t>(stream);
        values.append(value);
        break;
    case mandala::type_word:
        value.value = unpack<mandala::word_t>(stream);
        values.append(value);
        break;
    case mandala::type_byte:
        value.value = unpack<mandala::byte_t>(stream);
        values.append(value);
        break;
    case mandala::type_option:
        value.value = unpack<mandala::option_t>(stream);
        values.append(value);
        break;
    }
    if (value.value.isNull()) {
        values.clear();
        // error
        qDebug() << "error: " << Mandala::meta(pid.uid).path << stream.available()
                 << stream.dump_payload();
    }

    return values;
}

void ProtocolTelemetry::pack(const QVariant &v,
                             mandala::type_id_e type,
                             ProtocolStreamWriter &stream)
{
    switch (type) {
    default:
        return;
    case mandala::type_real:
        stream.write<mandala::real_t>(v.value<mandala::real_t>());
        return;
    case mandala::type_dword:
        stream.write<mandala::dword_t>(v.value<mandala::dword_t>());
        return;
    case mandala::type_word:
        stream.write<mandala::word_t>(v.value<mandala::word_t>());
        return;
    case mandala::type_byte:
        stream.write<mandala::byte_t>(v.value<mandala::byte_t>());
        return;
    case mandala::type_option:
        stream.write<mandala::option_t>(v.value<mandala::option_t>());
        return;
    }
}

void ProtocolTelemetry::sendValue(ProtocolTelemetry::TelemetryValue value)
{
    ostream.req(value.pid.uid, value.pid.pri);
    mandala::spec_s spec;
    spec.type = Mandala::meta(value.pid.uid).type_id;
    spec.write(&ostream);
    if (!value.value.isNull()) {
        pack(value.value, spec.type, ostream);
    }
    vehicle->send(ostream.toByteArray());
}
void ProtocolTelemetry::sendBundle(ProtocolTelemetry::TelemetryValues values)
{
    if (values.size() <= 1 && values.size() > 3) {
        qWarning() << "wrong size" << values.size();
        return;
    }

    ostream.req(values.first().pid.uid, values.first().pid.pri);
    mandala::spec_s spec;
    if (values.size() == 2)
        spec.type = mandala::type_vec2;
    else if (values.size() == 3)
        spec.type = mandala::type_vec3;

    spec.write(&ostream);

    for (auto const &v : values) {
        pack(v.value, Mandala::meta(v.pid.uid).type_id, ostream);
    }

    vehicle->send(ostream.toByteArray());
}