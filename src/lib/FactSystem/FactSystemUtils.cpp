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
#include <QtCore>
#include "FactSystemUtils.h"
//=============================================================================
FactSystemUtils::FactSystemUtils(QObject *parent)
 : QObject(parent)
{
}
//=============================================================================
QString FactSystemUtils::latToString(double v)
{
  double lat=fabs(v);
  double lat_m=60*(lat-floor(lat)),lat_s=60*(lat_m-floor(lat_m)),lat_ss=100*(lat_s-floor(lat_s));
  return QString().sprintf("%c %g%c%02g'%02g.%02g\"",(v>=0)?'N':'S',floor(lat),176,floor(lat_m),floor(lat_s),floor(lat_ss));
}
QString FactSystemUtils::lonToString(double v)
{
  double lat=fabs(v);
  double lat_m=60*(lat-floor(lat)),lat_s=60*(lat_m-floor(lat_m)),lat_ss=100*(lat_s-floor(lat_s));
  return QString().sprintf("%c %g%c%02g'%02g.%02g\"",(v>=0)?'E':'W',floor(lat),176,floor(lat_m),floor(lat_s),floor(lat_ss));
}
double FactSystemUtils::latFromString(QString s)
{
  bool ok;
  int i;
  s=s.simplified();
  if(QString("NS").contains(s.at(0))){
    bool bN=s.at(0)=='N';
    s=s.remove(0,1).trimmed();
    i=s.indexOf(QChar(176));
    double deg=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    s=s.remove(0,i+1).trimmed();
    i=s.indexOf('\'');
    double min=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    s=s.remove(0,i+1).trimmed();
    i=s.indexOf('\"');
    double sec=s.left(i).toDouble(&ok);
    if(!ok)return 0;
    deg=deg+min/60.0+sec/3600.0;
    return bN?deg:-deg;
  }
  return s.toDouble();
}
double FactSystemUtils::lonFromString(QString s)
{
  s=s.simplified();
  if(QString("EW").contains(s.at(0)))
    s[0]=(s.at(0)=='E')?'N':'S';
  return latFromString(s);
}
QString FactSystemUtils::distanceToString(uint v)
{
  if(v>=1000000)return QString("%1km").arg(v/1000.0,0,'f',0);
  if(v>=1000)return QString("%1km").arg(v/1000.0,0,'f',1);
  return QString("%1m").arg((uint)v);
}
QString FactSystemUtils::timeToString(uint v)
{
  if(v==0)return "--:--";
  qint64 d=(qint64)v/(24*60*60);
  if(d<=0)return QString("%1").arg(QTime(0,0,0).addSecs(v).toString("hh:mm"));
  return QString("%1d%2").arg(d).arg(QTime(0,0,0).addSecs(v).toString("hh:mm"));
}
uint FactSystemUtils::timeFromString(QString s)
{
  uint t=0;
  s=s.trimmed().toLower();
  if(s.contains('d')){
    QString ds=s.left(s.indexOf('d')).trimmed();
    s=s.remove(0,s.indexOf('d')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(24*60*60));
  }
  if(s.contains('h')){
    QString ds=s.left(s.indexOf('h')).trimmed();
    s=s.remove(0,s.indexOf('h')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60*60));
  }
  if(s.contains('m')){
    QString ds=s.left(s.indexOf('m')).trimmed();
    s=s.remove(0,s.indexOf('m')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60));
  }
  if(s.contains('s')){
    QString ds=s.left(s.indexOf('s')).trimmed();
    s=s.remove(0,s.indexOf('s')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv);
    s.clear();
  }
  if(s.contains(':')){
    QString ds=s.left(s.indexOf(':')).trimmed();
    s=s.remove(0,s.indexOf(':')+1).trimmed();
    bool ok=false;
    double dv=ds.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv*(double)(60*60));
    if(s.contains(':')){
      QString ds=s.left(s.indexOf(':')).trimmed();
      s=s.remove(0,s.indexOf(':')+1).trimmed();
      bool ok=false;
      double dv=ds.toDouble(&ok);
      if(ok && dv>0)t+=floor(dv*(double)(60));
    }else{
      bool ok=false;
      double dv=s.toDouble(&ok);
      if(ok && dv>0)t+=floor(dv*(double)(60));
      s.clear();
    }
  }
  if(!s.isEmpty()){
    bool ok=false;
    double dv=s.toDouble(&ok);
    if(ok && dv>0)t+=floor(dv);
  }
  return t;
}
//=============================================================================
void FactSystemUtils::toolTip(QString tooltip)
{
  qDebug()<<":: "<<tooltip;
}
double FactSystemUtils::limit(double v,double min,double max)
{
  if(v<min)return min;
  if(v>max)return max;
  return v;
}
double FactSystemUtils::angle(double v)
{
  const double span=180.0;
  const double dspan=span*2.0;
  return v-floor(v/dspan+0.5)*dspan;
}
double FactSystemUtils::angle360(double v)
{
  while(v<0) v+=360.0;
  while(v>=360.0) v-=360.0;
  return v;
}
double FactSystemUtils::angle90(double v)
{
  const double span=90.0;
  const double dspan=span*2.0;
  return v-floor(v/dspan+0.5)*dspan;
}
//=============================================================================
