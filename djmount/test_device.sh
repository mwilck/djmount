#!/bin/bash
# $Id: test_vfs.sh 249 2006-08-10 20:43:26Z r3mi $
# 
# Test Device object.
# This file is part of djmount.
# 
# (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 

fatal() {
    echo "" >&2
    echo "$*" >&2
    echo "**error** unexpected result" >&2
    exit 1
}


echo $0


#
echo -n " - standard description document ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion><major>1</major><minor>0</minor></specVersion>
<URLBase>http://192.168.28.1:9000/</URLBase>
<device>
    <dlna:X_DLNADOC xmlns:dlna="urn:schemas-dlna-org:device-1-0">DMS-1.50</dlna:X_DLNADOC><dlna:X_DLNACAP xmlns:dlna="urn:schema-dlna-org:device-1-0">av-upload,image-upload,audio-upload</dlna:X_DLNACAP>
    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>
    <UDN>uuid:89665984-7466-0011-2f58-320d2195</UDN>
    <friendlyName>Remi</friendlyName>
    <manufacturer>RemiVision GmbH</manufacturer>
    <manufacturerURL>http://djmount.sf.net</manufacturerURL>
    <modelName>RemiVision Media Server</modelName>
    <modelNumber>103</modelNumber>
    <modelURL>http://djmount.sf.net/test</modelURL>
    <modelDescription>the RemiVision Media Server</modelDescription>
    <serialNumber></serialNumber><UPC></UPC>
    <presentationURL>configpage/index.htm</presentationURL>
    <serviceList>
	<service><serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType><serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId><SCPDURL>ConnectionManager.xml</SCPDURL><eventSubURL>ConnectionManager/Event</eventSubURL><controlURL>ConnectionManager/Control</controlURL></service>
	<service><serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType><serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId><SCPDURL>ContentDirectory.xml</SCPDURL><eventSubURL>ContentDirectory/Event</eventSubURL><controlURL>ContentDirectory/Control</controlURL></service></serviceList>
</device>
</root>'

res="$(echo $xml | ./test_device 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:89665984-7466-0011-2f58-320d2195
  +- DeviceType     = urn:schemas-upnp-org:device:MediaServer:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = Remi
  +- PresURL        = http://192.168.28.1:9000/configpage/index.htm
  |
  +- Service
  |   |
  |   +- Class           = Service
  |   +- Object Name     = Service
  |   +- ServiceType     = urn:schemas-upnp-org:service:ConnectionManager:1
  |   +- ServiceId       = urn:upnp-org:serviceId:ConnectionManager
  |   +- EventURL        = http://192.168.28.1:9000/ConnectionManager/Event
  |   +- ControlURL      = http://192.168.28.1:9000/ConnectionManager/Control
  |   +- ServiceStateTable
  |   +- Last Action     = (null)
  |   +- SID             = (null)
  |
  +- Service
      |
      +- Class           = ContentDir
      +- Object Name     = ContentDir
      +- ServiceType     = urn:schemas-upnp-org:service:ContentDirectory:1
      +- ServiceId       = urn:upnp-org:serviceId:ContentDirectory
      +- EventURL        = http://192.168.28.1:9000/ContentDirectory/Event
      +- ControlURL      = http://192.168.28.1:9000/ContentDirectory/Control
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)
      +- Browse Cache
            +- Cache size      = 1024
            +- Cache max age   = 60 seconds
            +- Cached entries  = 0 (0%)
            +- Cache access    = 0
EOF

echo " OK"


#
echo -n " - same description document without URLBase ..."
#

xml=$(echo "$xml" | fgrep -v URLBase)

res="$(echo $xml | ./test_device 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:89665984-7466-0011-2f58-320d2195
  +- DeviceType     = urn:schemas-upnp-org:device:MediaServer:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = Remi
  +- PresURL        = http://test.url/configpage/index.htm
  |
  +- Service
  |   |
  |   +- Class           = Service
  |   +- Object Name     = Service
  |   +- ServiceType     = urn:schemas-upnp-org:service:ConnectionManager:1
  |   +- ServiceId       = urn:upnp-org:serviceId:ConnectionManager
  |   +- EventURL        = http://test.url/ConnectionManager/Event
  |   +- ControlURL      = http://test.url/ConnectionManager/Control
  |   +- ServiceStateTable
  |   +- Last Action     = (null)
  |   +- SID             = (null)
  |
  +- Service
      |
      +- Class           = ContentDir
      +- Object Name     = ContentDir
      +- ServiceType     = urn:schemas-upnp-org:service:ContentDirectory:1
      +- ServiceId       = urn:upnp-org:serviceId:ContentDirectory
      +- EventURL        = http://test.url/ContentDirectory/Event
      +- ControlURL      = http://test.url/ContentDirectory/Control
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)
      +- Browse Cache
            +- Cache size      = 1024
            +- Cache max age   = 60 seconds
            +- Cached entries  = 0 (0%)
            +- Cache access    = 0
EOF

echo " OK"


#
echo -n " - empty description document ..."
#

xml=''

res="$(echo $xml | ./test_device 2>&1 )" && fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Assertion)') <<EOF || fatal
[E] Device_Create no <device> in XML document = '
'
EOF

echo " OK"


#
echo -n " - invalid XML description document ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion><major>1</major><minor>0</minor></specVersion>
<device>
'

res="$(echo $xml | ./test_device 2>&1 )" && fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Assertion)') <<EOF || fatal
[E] Device_Create can't parse XML document (12) = '<?xml version="1.0"?> <root xmlns="urn:schemas-upnp-org:device-1-0"> <specVersion><major>1</major><minor>0</minor></specVersion> <device>
'
EOF

echo " OK"


#
echo -n " - invalid UTF-8 in otherwise correct description document ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion><major>1</major><minor>0</minor></specVersion>
<device>
    <dlna:X_DLNADOC xmlns:dlna="urn:schemas-dlna-org:device-1-0">DMS-1.50</dlna:X_DLNADOC><dlna:X_DLNACAP xmlns:dlna="urn:schema-dlna-org:device-1-0">av-upload,image-upload,audio-upload</dlna:X_DLNACAP>
    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>
    <UDN>uuid:89665984-7466-0011-2f58-320d2195</UDN>
    <friendlyName>Rémi</friendlyName>
    <manufacturer>RémiVision GmbH</manufacturer>
    <manufacturerURL>http://djmount.sf.net</manufacturerURL>
    <modelName>RémiVision Media Server</modelName>
    <modelNumber>103</modelNumber>
    <modelURL>http://djmount.sf.net/test</modelURL>
    <modelDescription>the RémiVision Media Server</modelDescription>
    <serialNumber></serialNumber><UPC></UPC>
    <presentationURL>configpage/index.htm</presentationURL>
    <serviceList>
	<service><serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType><serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId><SCPDURL>ConnectionManager.xml</SCPDURL><eventSubURL>ConnectionManager/Event</eventSubURL><controlURL>ConnectionManager/Control</controlURL></service>
	<service><serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType><serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId><SCPDURL>ContentDirectory.xml</SCPDURL><eventSubURL>ContentDirectory/Event</eventSubURL><controlURL>ContentDirectory/Control</controlURL></service></serviceList>
</device>
</root>'

res="$(echo $xml | ./test_device 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:89665984-7466-0011-2f58-320d2195
  +- DeviceType     = urn:schemas-upnp-org:device:MediaServer:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = R?mi
  +- PresURL        = http://test.url/configpage/index.htm
  |
  +- Service
  |   |
  |   +- Class           = Service
  |   +- Object Name     = Service
  |   +- ServiceType     = urn:schemas-upnp-org:service:ConnectionManager:1
  |   +- ServiceId       = urn:upnp-org:serviceId:ConnectionManager
  |   +- EventURL        = http://test.url/ConnectionManager/Event
  |   +- ControlURL      = http://test.url/ConnectionManager/Control
  |   +- ServiceStateTable
  |   +- Last Action     = (null)
  |   +- SID             = (null)
  |
  +- Service
      |
      +- Class           = ContentDir
      +- Object Name     = ContentDir
      +- ServiceType     = urn:schemas-upnp-org:service:ContentDirectory:1
      +- ServiceId       = urn:upnp-org:serviceId:ContentDirectory
      +- EventURL        = http://test.url/ContentDirectory/Event
      +- ControlURL      = http://test.url/ContentDirectory/Control
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)
      +- Browse Cache
            +- Cache size      = 1024
            +- Cache max age   = 60 seconds
            +- Cached entries  = 0 (0%)
            +- Cache access    = 0
EOF


echo " OK"


exit 0


