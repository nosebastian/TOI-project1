#!/bin/bash

BASEDIR=$(dirname "$0")

CONFIG_ADD=$'
[all]
dtoverlay=dwc2
'
CMDLINE_ADD=$' modules-load=dwc2'
MODULES_ADD=$'
libcomposite
'
DHCPD_ADD=$'
denyinterfaces usb0
'
USB0_ADD=$'
auto usb0
allow-hotplug usb0
iface usb0 inet static
  address 10.55.0.2
  gateway 10.55.0.1
  netmask 255.255.255.248
'
CONFIG_FILE="config.txt"
CMDLINE_FILE="cmdline.txt"
SSH_FILE="ssh"
MODULES_FILE="/etc/modules"
DHCPCD_FILE="/etc/dhcpcd.conf"
USB0_FILE="/etc/network/interfaces.d/usb0"

USB_SOURCE_FILE="$BASEDIR/rpiscripts/usb.sh"
USB_TARGET_FILE="/root/usb.sh"

RPI_SETUP_SOURCE_FILE="$BASEDIR/rpiscripts/rpisetup.sh"
RPI_SETUP_TARGET_FILE="/home/pi/rpisetup.sh"

SERVICE_CONTENT=$'
[Unit]
Description=Service for usb0 OTG link configuration

[Service]
Type=oneshot
ExecStart='$USB_TARGET_FILE'
StandardOutput=journal+console

[Install]
WantedBy=multi-user.target
'

SERVICE_FILE="/etc/systemd/system/usb0.service"
SERVICE_SIMLINK="/etc/systemd/system/multi-user.target.wants/usb0.service"

echo "RUNNING INSTALL SCRIPT WITHIN $BASEDIR"

if [ $USER != "root" ]
then
    echo "ERROR: Cannot run script as non root user" >&2
    echo "TRY: sudo !!" >&2
    exit -1
fi

if [ $# -ne 2 ]
then
    echo "ERROR: script needs 2 arguments" >&2
    echo "Usage:"
    echo ""
    echo "presetup.sh BOOT_DIR ROOT_DIR"
    echo "   BOOT_DIR - directory where boot sector of RPi sd card is mounted"
    echo "   ROOT_DIR - directory where root sector of RPi sd card is mounted"
    exit 1
elif [ -d $1 ] && [ -d $2 ]
then
#BOOT DIRECTORY CONFIGURATION
    #Add device tree overlay to config.txt
    CONFIG_PATH="$1/$CONFIG_FILE"
    echo -n "$CONFIG_ADD" >> $CONFIG_PATH
    #Add cmdline option of device tree overlay
    CMDLINE_PATH="$1/$CMDLINE_FILE"
    echo -n "$CMDLINE_ADD" >> $CMDLINE_PATH
    #Configure ssh
    SSH_PATH="$1/$SSH_FILE"
    touch $SSH_PATH

#ROOT DIRECTORY CONFIGURATION
    if [ -f $USB_SOURCE_FILE ]
    then
        #Copy usb.sh file to root directory
        USB_FILE_PATH="$2/$USB_TARGET_FILE"
        cp $USB_SOURCE_FILE $USB_FILE_PATH
        chmod 755 $USB_FILE_PATH
        chown root:root $USB_FILE_PATH
        #Setup service and simlinks
        SEVICE_PATH="$2/$SERVICE_FILE"
        echo -n "$SERVICE_CONTENT" > $SEVICE_PATH
        chmod 644 $SEVICE_PATH
        chown root:root $SEVICE_PATH
        SERVICE_SIMLINK_PATH="$2/$SERVICE_SIMLINK"
        ln -s -f $SERVICE_FILE $SERVICE_SIMLINK_PATH
        #Add the modules config
        MODULES_PATH="$2/$MODULES_FILE"
        echo -n "$MODULES_ADD" >> $MODULES_PATH
        #Add dhcpcd config for usb0
        DHCPCD_PATH="$2/$DHCPCD_FILE"
        echo -n "$DHCPD_ADD" >> $DHCPCD_PATH
        #Add interfaces config for usb0
        USB0_PATH="$2/$USB0_FILE"
        echo -n "$USB0_ADD" > $USB0_PATH
#RPI HOME CONFIG
    if [ -f $RPI_SETUP_SOURCE_FILE ]
    then
        RPI_SETUP_PATH="$2/$RPI_SETUP_TARGET_FILE"
        cp $RPI_SETUP_SOURCE_FILE $RPI_SETUP_PATH
        chmod 755 $RPI_SETUP_PATH
        chown root:root $RPI_SETUP_PATH
    else
        echo "ERROR: could not find rpisetup.sh in script directory" >&2
        exit 4
    fi
    else
        echo "ERROR: could not find usb.sh in script directory" >&2
        exit 3
    fi
else
    echo "ERROR: invalid options" >&2
    echo "Usage:"
    echo ""
    echo "presetup.sh BOOT_DIR ROOT_DIR"
    echo "   BOOT_DIR - directory where boot sector of RPi sd card is mounted"
    echo "   ROOT_DIR - directory where root sector of RPi sd card is mounted"
    exit 2
fi

exit 0
