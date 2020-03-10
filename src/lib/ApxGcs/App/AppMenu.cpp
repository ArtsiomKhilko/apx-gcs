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
#include "AppMenu.h"

#include <App/App.h>
#include <App/AppGcs.h>
#include <App/AppRoot.h>
#include <ApxMisc/QActionFact.h>
#include <Fact/FactListModel.h>

#include <Nodes/Nodes.h>
#include <Nodes/NodesShare.h>

#include <Datalink/Datalink.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicles.h>

#include <QDesktopServices>
//=============================================================================
AppMenu::AppMenu(Fact *parent)
    : Fact(parent, "sysmenu", tr("Menu"), tr("Application system menu"), Action | IconOnly, "menu")
    , menuBar(nullptr)
{
    setOpt("pos", QPointF(1, 0.3));

    Fact *f;

    app = new Fact(this, "app", tr("Application"), "", Group, "application");
    f = new Fact(app,
                 "about",
                 QString("%1 %2").arg(tr("About")).arg(QCoreApplication::applicationName()),
                 "",
                 NoFlags,
                 "information-outline");
    f->setOpt("role", QAction::AboutRole);
    connect(f, &Fact::triggered, App::instance(), &App::about);

    f = new Fact(app, "preferences", tr("Preferences"), "", NoFlags, "settings-outline");
    f->setOpt("role", QAction::PreferencesRole);
    connect(f, &Fact::triggered, AppRoot::instance(), &Fact::trigger);

    if (!App::installed()) {
        f = new Fact(app, "install", tr("Install application"), "", NoFlags, "package-variant");
        f->setOpt("role", QAction::ApplicationSpecificRole);
        connect(f, &Fact::triggered, App::instance(), &App::install);
    }

    file = new Fact(this, "file", tr("File"), "", Group, "file");
    f = new Fact(file, "telemetry");
    f->setOpt("shortcut", QKeySequence::Open);
    f->bind(Vehicles::instance()->f_replay->f_telemetry->f_share->f_import);
    f = new Fact(file, "nodes");
    // FIXME: f->bind(Vehicles::instance()->f_replay->f_nodes->f_share->f_import);
    f = new Fact(file, "datalink");
    f->bind(AppGcs::instance()->f_datalink);
    f->setSection(tr("Communication"));

    vehicles = new Fact(this, "vehicles", tr("Vehicles"), "", Group | FlatModel, "airplane");
    vehicleSelect = new Fact(vehicles, "select", tr("Select vehicle"), "", Group | Section);
    vehicleTools = new Fact(vehicles, "vehicle", tr("Current vehicle"), "", Group | Section);
    connect(Vehicles::instance(),
            &Vehicles::currentChanged,
            this,
            &AppMenu::updateVehicleTools,
            Qt::QueuedConnection);
    updateVehicleTools();
    connect(Vehicles::instance()->f_select,
            &Fact::itemInserted,
            this,
            &AppMenu::updateVehicleSelect,
            Qt::QueuedConnection);
    connect(Vehicles::instance()->f_select,
            &Fact::itemRemoved,
            this,
            &AppMenu::updateVehicleSelect,
            Qt::QueuedConnection);
    updateVehicleSelect();

    tools = new Fact(this, "tools", "", "", Group);
    tools->bind(AppRoot::instance()->f_tools);

    windows = new Fact(this, "windows", "", "", Group);
    windows->bind(AppRoot::instance()->f_windows);

    help = new Fact(this, "help", tr("Help"), "", Group, "help");
    f = new Fact(help, "mrep", tr("Mandala Report"), "", NoFlags, "format-list-bulleted");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://127.0.0.1:9080/mandala?descr"));
    });
    f = new Fact(help, "docs", tr("Documentation"), "", NoFlags, "help-circle-outline");
    f->setOpt("shortcut", QKeySequence::HelpContents);
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://docs.uavos.com"));
    });
    f = new Fact(help, "changelog", tr("Changelog"), "", NoFlags, "delta");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("http://uavos.github.io/apx-releases/CHANGELOG.html"));
    });
    f = new Fact(help, "issue", tr("Report a problem"), "", NoFlags, "bug-outline");
    connect(f, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/uavos/apx-releases/issues"));
    });

    connect(App::instance(), &App::loadingFinished, this, &AppMenu::createMenuBar);
}
//=============================================================================
void AppMenu::updateVehicleTools()
{
    Vehicle *v = Vehicles::instance()->current();
    if (!v)
        return;
    vehicleTools->removeAll();
    for (int i = 0; i < v->size(); ++i) {
        Fact *f = v->child(i);
        Fact *a = new Fact(vehicleTools, f->name());
        a->bind(f);
    }
}
void AppMenu::updateVehicleSelect()
{
    Fact *v = Vehicles::instance()->f_select;
    vehicleSelect->removeAll();
    for (int i = 0; i < v->size(); ++i) {
        Fact *f = v->child(i);
        Fact *a = new Fact(vehicleSelect, f->name(), f->title(), f->descr(), Bool);
        a->setValue(f->active());
        connect(f, &Fact::activeChanged, a, [a, f]() { a->setValue(f->active()); });
        connect(a, &Fact::triggered, f, &Fact::trigger);
    }
}
//=============================================================================
void AppMenu::createMenuBar()
{
    if (menuBar)
        delete menuBar;

    menuBar = new QMenuBar(nullptr);
    for (int i = 0; i < size(); ++i) {
        Fact *g = child(i);
        Fact *f = g->menu();
        if (!f)
            continue;
        QMenu *menu = menuBar->addMenu(g->title());
        f->setOpt("qmenu", QVariant::fromValue(menu));
        updateMenu(f);
        QAbstractListModel *model = f->model();
        if (model) {
            connect(
                model,
                &QAbstractListModel::layoutChanged,
                this,
                [this, f]() { updateMenu(f); },
                Qt::QueuedConnection);
        }
    }
}
void AppMenu::updateMenu(Fact *fact)
{
    QMenu *menu = fact->opts().value("qmenu").value<QMenu *>();
    if (!menu)
        return;

    menu->clear();

    FactListModel *model = qobject_cast<FactListModel *>(fact->model());
    if (!model)
        return;
    model->sync();

    QString sect;
    for (int i = 0; i < model->count(); ++i) {
        Fact *f = model->get(i);
        Fact *fm = f->menu();
        if (fm)
            f = fm;
        if (sect != f->section()) {
            sect = f->section();
            menu->addSection(sect);
        }
        QAction *a = new QActionFact(f, QColor(Qt::black));
        menu->addAction(a);
    }
}
//=============================================================================
