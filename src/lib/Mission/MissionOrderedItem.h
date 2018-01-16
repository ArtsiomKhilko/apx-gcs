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
#ifndef MissionOrderedItem_H
#define MissionOrderedItem_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "MissionItems.h"
//=============================================================================
class MissionOrderedItem: public Fact
{
  Q_OBJECT
  Q_PROPERTY(int missionItemType READ missionItemType CONSTANT)

public:
  explicit MissionOrderedItem(MissionItems *parent, const QString &name, const QString &title, const QString &descr);

  MissionItems *missionItems;

  int missionItemType() const;

protected:
  template<class T> T prevItem() const
  {
    return static_cast<T>(parentItem()->child(num()-1));
  }

  template<class T> T nextItem() const
  {
    return static_cast<T>(parentItem()->child(num()+1));
  }

private:
  QString namePrefix;

private slots:
  virtual void updateTitle();
};
//=============================================================================
#endif

