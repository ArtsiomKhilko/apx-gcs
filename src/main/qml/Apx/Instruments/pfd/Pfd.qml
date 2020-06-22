﻿import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1

import APX.Vehicles 1.0
import "."
import "../common"

Item {
    id: pfd

    readonly property var f_mode: mandala.cmd.op.mode

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property var f_cmd_airspeed: mandala.cmd.air.airspeed
    readonly property var f_cmd_altitude: mandala.cmd.pos.altitude

    readonly property var f_gps_src: mandala.sns.gps.src
    readonly property var f_gps_status: mandala.sns.gps.status
    readonly property var f_gps_emi: mandala.sns.gps.emi
    readonly property var f_gps_su: mandala.sns.gps.su
    readonly property var f_gps_sv: mandala.sns.gps.sv
    readonly property var f_ref_status: mandala.est.ref.status

    readonly property var f_ktas: mandala.est.air.ktas
    readonly property var f_ld: mandala.est.air.ld

    readonly property var f_thrcut: mandala.cmd.opt.thrcut
    readonly property var f_throvr: mandala.cmd.opt.throvr
    readonly property var f_thr: mandala.ctr.eng.thr
    readonly property var f_rc_thr: mandala.cmd.rc.thr

    readonly property var f_vsys: mandala.sns.pwr.vsys
    readonly property var f_vsrv: mandala.sns.pwr.vsrv
    readonly property var f_vpld: mandala.sns.pwr.vpld
    readonly property var f_veng: mandala.sns.eng.voltage

    readonly property var f_air_temp: mandala.sns.air.temp
    readonly property var f_rt: mandala.sns.aux.rt

    readonly property var f_rpm: mandala.sns.eng.rpm
    readonly property var f_airbrk: mandala.ctr.wing.airbrk

    readonly property var f_baro_status: mandala.sns.baro.status
    readonly property var f_pwr_status: mandala.sns.pwr.status

    readonly property var f_pitot_status: mandala.sns.pitot.status

    readonly property var f_bat_status: mandala.sns.bat.status

    readonly property var f_eng_status: mandala.sns.eng.status
    readonly property var f_eng_tc: mandala.sns.eng.tc
    readonly property var f_eng_starter: mandala.ctr.eng.starter

    readonly property var f_pwr_servo: mandala.ctr.pwr.servo
    readonly property var f_pwr_payload: mandala.ctr.pwr.payload

    readonly property var f_ers_status: mandala.sns.ers.status
    readonly property var f_ers_block: mandala.sns.ers.block

    readonly property var f_ahrs_status: mandala.est.ahrs.status
    readonly property var f_ahrs_stall: mandala.est.ahrs.stall
    readonly property var f_ahrs_inair: mandala.est.ahrs.inair
    readonly property var f_ahrs_imu: mandala.est.ahrs.imu

    readonly property var f_rc_ovr: mandala.cmd.rc.ovr


    readonly property var f_ref_altitude: mandala.est.ref.altitude

    readonly property var f_ctr_hover: mandala.est.ctr.hover


    clip: true

    implicitWidth: 600
    implicitHeight: 300

    readonly property url pfdImageUrl: Qt.resolvedUrl("pfd.svg")

    property bool showHeading: true
    property bool showWind: true
    property alias flagHeight: pfdScene.flagHeight

    readonly property bool m_err_pwr: f_pwr_status.value > pwr_status_ok

    Rectangle {
        color: "#777"
        border.width: 0
        anchors.fill: parent
    }

    Item {
        id: pfdScene

        width: parent.width
        height: parent.height
        anchors.centerIn: parent

        readonly property real txtHeight: apx.limit(left_window.width*0.2,0,parent.height*0.1)
        readonly property real flagHeight: txtHeight*0.65
        readonly property real topFramesMargin: (width-width*0.6)*0.6*0.2

        readonly property color power_color: (ui.test||m_err_pwr)?"red":"transparent"

        Horizon {
            id: horizon
            margin_left: 0.2
            margin_right: 0.3
            showHeading: pfd.showHeading
        }

        Wind {
            anchors.right: right_window.left
            anchors.bottom: parent.bottom
            anchors.rightMargin: right_window.width*0.2
            anchors.bottomMargin: parent.height*0.2
            width: parent.width*0.05
            height: width
            value: m_whdg-f_yaw.value
        }

        ILS {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: parent.width*(horizon.margin_left-horizon.margin_right)/2
            width: parent.width*0.3
            height: apx.limit(width,0,parent.height*0.6)
        }

        //flight mode text
        Text {
            color: "white"
            anchors.top: parent.top
            anchors.left: parent.horizontalCenter
            anchors.right: right_window.left
            anchors.topMargin: pfdScene.flagHeight*1.5
            text: f_mode.text
            font.pixelSize: pfdScene.txtHeight
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            ToolTipArea { text: f_mode.title }
        }



        Rectangle {
            id: left_window
            color: "transparent"
            border.width: 0
            anchors.fill: parent
            anchors.rightMargin: parent.width*0.8
            anchors.leftMargin: 2

            Airspeed {
                id: speed_window
                anchors.fill: parent
                anchors.leftMargin: parent.width*0.6
                anchors.topMargin: pfdScene.topFramesMargin
                anchors.bottomMargin: anchors.topMargin
            }

            RectNum {
                value: f_cmd_airspeed.value.toFixed()
                toolTip: f_cmd_airspeed.title
                anchors.left: speed_window.left
                anchors.right: speed_window.right
                anchors.top: parent.top
                anchors.bottom: speed_window.top
                anchors.topMargin: 3
                anchors.leftMargin: parent.width*0.1
            }

            Flags {
                id: flags
                txtHeight: pfdScene.flagHeight
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.left: parent.left
                anchors.leftMargin: 1
            }

            Column {
                id: modeFlags
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.left: parent.left
                readonly property real modeHeight: pfdScene.flagHeight

                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_vsrv
                    title: "Vs"
                    show: value>1 || failure
                    precision: 1
                    type_default: f_pwr_servo.value>0?CleanText.Clean:CleanText.Normal
                    failure: m_err_pwr
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_veng
                    title: "Vm"
                    show: value>1 || failure
                    precision: 1
                    failure: m_err_pwr
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_vpld
                    title: "Vp"
                    show: value>1 || failure
                    precision: 1
                    type_default: f_pwr_payload.value>0?CleanText.Clean:CleanText.Normal
                    failure: m_err_pwr
                }

                CleanText {
                    height: pfdScene.flagHeight

                    fact: f_ers_status
                    readonly property int status: f_ers_status.value
                    readonly property bool blocked: f_ers_block.value > 0
                    readonly property bool ok: status == ers_status_ok
                    readonly property bool disarmed: status == ers_status_disarmed

                    visible: ui.test || status > ers_status_ok || blocked

                    type: disarmed
                           ? blocked
                             ? CleanText.Green
                             : CleanText.Black
                           : CleanText.Red
                    prefix: qsTr("ERS")
                }

                NumberText {
                    id: at_num
                    height: modeFlags.modeHeight
                    fact: f_air_temp
                    title: "AT"
                    show: false
                    onValueChanged: show = (show || value !== 0)
                }
                NumberText {
                    id: rt_num
                    height: modeFlags.modeHeight
                    fact: f_rt
                    title: "RT"
                    show: false
                    onValueChanged: show = (show || value !== 0)
                    warning: value>=50
                    failure: value>=70
                    blinking: value>=60
                }

                BlinkingText {
                    height: modeFlags.modeHeight
                    prefix: qsTr("EMI")
                    fact: f_gps_emi

                    readonly property int emi: fact.value

                    visible: ui.test || emi > gps_emi_ok

                    type: (emi === gps_emi_warning)
                           ? CleanText.Normal
                           : CleanText.Red

                    blinking: emi > gps_emi_warning
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_gps_status
                    title: qsTr("GPS")
                    toolTip: f_gps_status.title+", "+f_gps_su.title+"/"+f_gps_sv.title

                    readonly property int status: f_gps_status.value
                    readonly property int su: f_gps_su.value
                    readonly property int sv: f_gps_sv.value
                    readonly property bool ref: f_ref_status.value === ref_status_initialized
                    readonly property bool avail: status !== gps_status_nofix

                    readonly property bool isOff: (!avail) && (!ref)
                    readonly property bool isErr: ref && (!avail)
                    readonly property bool isOk:  ref && su>4 && su<=sv && (sv/su)<1.8 && status >= gps_status_3D

                    show: f_gps_src.value > gps_src_unknown

                    type_default: ref?CleanText.Clean:CleanText.Normal
                    failure: isErr

                    textColor: isOk?"white":"yellow"

                    text: su+"/"+sv +(
                              (!avail || status === gps_status_3D)
                              ? ""
                              : (" "+f_gps_status.text)
                              )
                }
            }
        }

        Rectangle {
            id: right_window
            color: "transparent"
            border.width: 0
            anchors.fill: parent
            anchors.leftMargin: parent.width*0.7


            Altitude {
                id: altitude_window
                anchors.fill: parent
                anchors.rightMargin: parent.width*0.5
                anchors.topMargin: pfdScene.topFramesMargin
                anchors.bottomMargin: (parent.width-parent.width*0.5)*0.3

                CleanText { // in air
                    anchors.verticalCenterOffset: pfdScene.flagHeight*1.5
                    anchors.centerIn: parent
                    visible: ui.test || ( fact.value === ahrs_inair_landed && f_ahrs_status.value > ahrs_status_unknown)
                    height: pfdScene.txtHeight*0.7
                    fact: f_ahrs_inair
                }
                StatusFlag { // baro status
                    anchors.verticalCenterOffset: -pfdScene.flagHeight*1.5
                    anchors.centerIn: parent
                    height: pfdScene.flagHeight
                    fact: f_baro_status
                    status_warning: baro_status_warning
                }

            }


            Vspeed {
                anchors.left: altitude_window.right
                anchors.right: parent.right
                height: altitude_window.height
                anchors.leftMargin: altitude_window.width*0.05
            }

            RectNum {
                value: f_cmd_altitude.value.toFixed()
                toolTip: f_cmd_altitude.title
                anchors.left: altitude_window.left
                anchors.right: altitude_window.right
                anchors.top: parent.top
                anchors.bottom: altitude_window.top
                anchors.topMargin: 3
            }
            Row {
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2

                NumberText {
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    show: ui.test || value>0
                    fact: f_ref_altitude
                    title: qsTr("MSL")
                }
            }
            NumberText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.rightMargin: 4
                height: pfdScene.txtHeight
                show: ui.test || value>0
                fact: f_ld
                title: qsTr("LD")
                precision: 1
            }

        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 4
            anchors.left: left_window.right
            anchors.leftMargin: 4
            spacing: 4

            StatusFlag {
                id: hoverFlag
                height: pfdScene.flagHeight
                fact: f_ctr_hover
                status_show: 1
                type: CleanText.Green
                text: fact.name
            }
            StatusFlag {
                id: airbrkFlag
                height: pfdScene.flagHeight
                fact: f_airbrk
                readonly property real v: fact.value
                show: v > 0
                text: qsTr("AIRBR")
                status_warning: 0.3
                status_failure: 0.7
                CleanText {
                    readonly property real v: airbrkFlag.v
                    fact: airbrkFlag.fact
                    show: (v>0 && v<1)
                    height: pfdScene.flagHeight
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (v*100).toFixed()
                }
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            anchors.left: left_window.right
            anchors.leftMargin: 4
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_ktas
                readonly property real v: f_ktas.value
                show: (v!==0) && (v<0.5||v>1.8)
                type: CleanText.Yellow
                text: qsTr("TAS")
            }
            StatusFlag {
                id: flag_CAS
                height: pfdScene.flagHeight
                fact: f_pitot_status
                status_warning: pitot_status_warning
                status_reset: pitot_status_unknown
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.verticalCenter
            anchors.bottomMargin: 4
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_ahrs_stall
                status_warning: ahrs_stall_warning
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_ahrs_status
                status_warning: ahrs_status_warning
                status_show: ahrs_status_busy
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_ahrs_imu
                status_warning: ahrs_imu_warning
            }
            StatusFlag {
                anchors.topMargin: pfdScene.flagHeight*2
                height: pfdScene.flagHeight
                fact: f_bat_status
                status_warning: bat_status_warning
                status_show: bat_status_shutdown
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_rc_ovr
                text: qsTr("RC")
                status_show: rc_ovr_on
                blinking: true
                type: CleanText.Yellow
            }
        }

        Text {
            id: offline
            color: "#80000000"
            anchors.bottom: parent.verticalCenter
            anchors.top: parent.top
            anchors.left: left_window.right
            anchors.right: right_window.left
            text: qsTr("OFFLINE")
            visible: !apx.vehicles.current.protocol.isReplay && !apx.datalink.online
            font.pixelSize: apx.datalink.valid?(parent.height*0.5*0.35):10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
            font.bold: true
        }
        Text {
            id: xpdrData
            color: "#60000000"
            anchors.bottom: parent.verticalCenter
            anchors.left: left_window.right
            anchors.right: right_window.left
            text: apx.vehicles.current.protocol.streamType===Vehicle.XPDR?qsTr("XPDR"):qsTr("NO DATA")
            visible: !apx.vehicles.current.protocol.isReplay && apx.datalink.valid && (apx.vehicles.current.protocol.streamType!==Vehicle.TELEMETRY)
            font.pixelSize: parent.height*0.5*0.25
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            font.bold: true
        }

        Item{
            id: center_numbers
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            width: parent.width*0.33
            NumberText {
                id: rpm_number
                readonly property int status: f_eng_status.value
                readonly property bool ok: status > eng_status_unknown
                readonly property bool warn: status == eng_status_warning
                readonly property bool err: status > eng_status_warning
                visible: ui.test || ok || err || value>0
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_rpm
                title: qsTr("RPM")
                value: fact.value/(precision>0?1000:1)
                precision: 0
                toolTip: fact.title + (precision>0?"[x1000]":"")
                warning: warn||!ok
                failure: err
            }
            Column {
                anchors.bottom: rpm_number.top
                anchors.bottomMargin: 1
                anchors.left: rpm_number.left

                BlinkingText { // turbocharger
                    property int v: fact.value
                    visible: ui.test || v > eng_tc_off
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_tc
                    type: v >= eng_tc_warning
                           ? CleanText.Red
                           : CleanText.Clean
                    blinking: v > eng_tc_warning
                }
                BlinkingText { // engine status
                    property int v: fact.value
                    property bool ctr: f_eng_starter.value>0
                    visible: ui.test || ctr || (v > eng_status_unknown && v < eng_status_running)
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_status
                    type: ctr
                           ? (v == eng_status_start ? CleanText.Green : CleanText.Red )
                           : CleanText.Clean
                    blinking: ctr
                }
            }

            NumberText {
                id: thr_number
                anchors.left: parent.horizontalCenter
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_thr
                title: qsTr("T")
                readonly property bool throvr: f_throvr.value
                readonly property bool thrcut: f_thrcut.value
                text: thrcut?qsTr("CUT"):(value*100).toFixed()
                toolTip: fact.title +"[x100]"+", "+f_thrcut.title+" ("+qsTr("red")+"), "+f_throvr.title+" ("+qsTr("blue")+")"
                show: true
                blinking: value>=0.98
                type: throvr
                      ? CleanText.Blue
                      : thrcut
                        ? CleanText.Red
                        : value >= 0.9
                            ? CleanText.Normal
                            : CleanText.Clean
            }
            NumberText {
                id: rc_thr_number
                anchors.right: thr_number.left
                anchors.rightMargin: 4
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_rc_thr
                title: qsTr("R")
                text: (value*100).toFixed()
                show: value>0 //&& (value!=(f_thr.value*100).toFixed())
                textColor: "magenta"
                type_default: CleanText.Normal
                toolTip: f_rc_thr.title +"[x100]"
            }

            NumberText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_vsys
                title: qsTr("V")
                show: value>0 || failure
                precision: 1
                failure: m_err_pwr
            }
        }
    }
}
