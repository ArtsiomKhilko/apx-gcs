/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <Fact/Fact.h>
#include <Sharing/Share.h>
#include <QtCore>

#include <ApxMisc/QueueJob.h>

class Telemetry;

class TelemetryShare : public Share
{
    Q_OBJECT

public:
    explicit TelemetryShare(Telemetry *telemetry,
                            Fact *parent,
                            FactBase::Flags flags = FactBase::Flags(Action | IconOnly));

    Fact *f_stop;

private:
    Telemetry *_telemetry;

    QueueJob *qimp;
    QueueJob *qexp;
    QString descr_s;

    QString getDefaultTitle() override;
    bool exportRequest(QString format, QString fileName) override;
    bool importRequest(QString format, QString fileName) override;

    void syncTemplates() override;

private slots:
    void updateActions();
    void updateProgress();
    void updateStatus();
    void updateDescr();

    void syncTemplate(QString hash);

signals:
    void importJobDone(quint64 id);
};
