####
# canmapd install script
#
# Author: Tobias Schmitt
# E-Mail: tobias.schmitt@codm.de
####

PROGRAMDIR=/usr/local/bin
SYSTEMDDIR=/usr/lib/systemd/system

echo "1/ close services"
systemctl stop canmapd.service
systemctl disable canmapd.service

echo "2/ check for $PROGRAMDIR"
if [ ! -d "$PROGRAMDIR" ]; then
    echo "not available, create"
    mkdir -p $PROGRAMDIR
fi

echo "3/ check for $SYSTEMDDIR"
if [ ! -d "$SYSTEMDDIR" ]; then
    echo "not available, create"
    mkdir -p $SYSTEMDDIR
fi

echo "4/ make"
make

echo "5/ copy files"
cp canmapd $PROGRAMDIR
cp canmapd.service $SYSTEMDDIR

echo "6/ restart services"
systemctl enable canmapd.service
systemctl start canmapd.service
