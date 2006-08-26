#!/bin/bash
# $Id$
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
echo -n " - same description document but unknown UDN ..."
#

res="$(echo $xml | ./test_device uuid:unknown 2>&1 )" || fatal "$res"

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
echo -n " - device with no services ..."
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

EOF

echo " OK"


#
echo -n " - description document with superfluous comments ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion><major>1</major><minor>0</minor></specVersion>
<URLBase>http://192.168.28.1:9000/</URLBase>
<device>
    <!-- x -->
    <deviceType><!-- -->urn:schemas-upnp-org:device:MediaServer:1</deviceType>
    <UDN><!-- x -->uuid:89665984-7466-0011-2f58-320d2195<!-- x --></UDN>
    <friendlyName><!-- x -->Remi<!-- x -->xyz<!-- x --></friendlyName>
    <manufacturer>RemiVision GmbH</manufacturer>
    <manufacturerURL>http://djmount.sf.net</manufacturerURL>
    <modelName>RemiVision Media Server</modelName>
    <modelNumber>103</modelNumber>
    <modelURL>http://djmount.sf.net/test</modelURL>
    <modelDescription>the RemiVision Media Server</modelDescription>
    <serialNumber></serialNumber><UPC></UPC>
    <presentationURL><!-- x -->configpage/index.htm</presentationURL>
    <serviceList>
	<!-- x -->
	<service><serviceType><!-- x -->urn:schemas-upnp-org:service:ConnectionManager:1</serviceType><serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId><SCPDURL>ConnectionManager.xml</SCPDURL><eventSubURL>ConnectionManager/Event</eventSubURL><controlURL>ConnectionManager/Control</controlURL></service>
	<!-- x -->
	<service><serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType><serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId><SCPDURL>ContentDirectory.xml</SCPDURL><eventSubURL><!-- x -->ContentDirectory/Event<!-- x --></eventSubURL><controlURL>ContentDirectory/Control</controlURL></service>
    </serviceList>
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
#'

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



#
echo -n " - description document with embedded devices (root) ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion>
<major>1</major>
<minor>0</minor>
</specVersion>
<URLBase>http://192.168.1.1:5431</URLBase>
<device>
<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>
<friendlyName>My Router</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e500a80c00</UDN>
<presentationURL>http://192.168.1.1/UPnP.asp</presentationURL>
<serviceList>
<service>
<serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>
<serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>
<controlURL>/uuid:0012-1707-c2e500a80c00/Layer3Forwarding:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e500a80c00/Layer3Forwarding:1</eventSubURL>
<SCPDURL>/dynsvc/Layer3Forwarding:1.xml</SCPDURL>
</service>
</serviceList>
<deviceList>
<device>
<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>
<friendlyName>Embedded_1</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e501d00c00</UDN>
<serviceList>
<service>
<serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>
<controlURL>/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1</eventSubURL>
<SCPDURL>/dynsvc/WANCommonInterfaceConfig:1.xml</SCPDURL>
</service>
</serviceList>
<deviceList>
<device>
<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>
<friendlyName>Embedded_1_1</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e502100c00</UDN>
<serviceList>
<service>
<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>
<controlURL>/uuid:0012-1707-c2e502100c00/WANIPConnection:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e502100c00/WANIPConnection:1</eventSubURL>
<SCPDURL>/dynsvc/WANIPConnection:1.xml</SCPDURL>
</service>
<service>
<serviceType>urn:schemas-upnp-org:service:WANPPPConnection:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANPPPConn1</serviceId>
<controlURL>/uuid:0012-1707-c2e502100c00/WANPPPConnection:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e502100c00/WANPPPConnection:1</eventSubURL>
<SCPDURL>/dynsvc/WANPPPConnection:1.xml</SCPDURL>
</service>
</serviceList>
</device>
</deviceList>
</device>
</deviceList>
</device>
</root>
'

res="$(echo $xml | ./test_device 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:0012-1707-c2e500a80c00
  +- DeviceType     = urn:schemas-upnp-org:device:InternetGatewayDevice:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = My Router
  +- PresURL        = http://192.168.1.1/UPnP.asp
  |
  +- Service
      |
      +- Class           = Service
      +- Object Name     = Service
      +- ServiceType     = urn:schemas-upnp-org:service:Layer3Forwarding:1
      +- ServiceId       = urn:upnp-org:serviceId:L3Forwarding1
      +- EventURL        = http://192.168.1.1:5431/uuid:0012-1707-c2e500a80c00/Layer3Forwarding:1
      +- ControlURL      = http://192.168.1.1:5431/uuid:0012-1707-c2e500a80c00/Layer3Forwarding:1
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)
EOF


echo " OK"


#
echo -n " - description document with embedded devices (2) ..."
#


res="$(echo $xml | ./test_device uuid:0012-1707-c2e501d00c00 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:0012-1707-c2e501d00c00
  +- DeviceType     = urn:schemas-upnp-org:device:WANDevice:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = Embedded_1
  +- PresURL        = 
  |
  +- Service
      |
      +- Class           = Service
      +- Object Name     = Service
      +- ServiceType     = urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1
      +- ServiceId       = urn:upnp-org:serviceId:WANCommonIFC1
      +- EventURL        = http://192.168.1.1:5431/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1
      +- ControlURL      = http://192.168.1.1:5431/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)

EOF


echo " OK"


#
echo -n " - description document with embedded devices (3) ..."
#


res="$(echo $xml | ./test_device uuid:0012-1707-c2e502100c00 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:0012-1707-c2e502100c00
  +- DeviceType     = urn:schemas-upnp-org:device:WANConnectionDevice:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = Embedded_1_1
  +- PresURL        = 
  |
  +- Service
  |   |
  |   +- Class           = Service
  |   +- Object Name     = Service
  |   +- ServiceType     = urn:schemas-upnp-org:service:WANIPConnection:1
  |   +- ServiceId       = urn:upnp-org:serviceId:WANIPConn1
  |   +- EventURL        = http://192.168.1.1:5431/uuid:0012-1707-c2e502100c00/WANIPConnection:1
  |   +- ControlURL      = http://192.168.1.1:5431/uuid:0012-1707-c2e502100c00/WANIPConnection:1
  |   +- ServiceStateTable
  |   +- Last Action     = (null)
  |   +- SID             = (null)
  |
  +- Service
      |
      +- Class           = Service
      +- Object Name     = Service
      +- ServiceType     = urn:schemas-upnp-org:service:WANPPPConnection:1
      +- ServiceId       = urn:upnp-org:serviceId:WANPPPConn1
      +- EventURL        = http://192.168.1.1:5431/uuid:0012-1707-c2e502100c00/WANPPPConnection:1
      +- ControlURL      = http://192.168.1.1:5431/uuid:0012-1707-c2e502100c00/WANPPPConnection:1
      +- ServiceStateTable
      +- Last Action     = (null)
      +- SID             = (null)

EOF


echo " OK"


#
echo -n " - description document with embedded devices and no root service ..."
#

xml='<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion>
<major>1</major>
<minor>0</minor>
</specVersion>
<URLBase>http://192.168.1.1:5431</URLBase>
<device>
<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>
<friendlyName>My Router</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e500a80c00</UDN>
<presentationURL>http://192.168.1.1/UPnP.asp</presentationURL>
<deviceList>
<device>
<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>
<friendlyName>urn:schemas-upnp-org:device:WANDevice:1</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e501d00c00</UDN>
<serviceList>
<service>
<serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>
<controlURL>/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e501d00c00/WANCommonInterfaceConfig:1</eventSubURL>
<SCPDURL>/dynsvc/WANCommonInterfaceConfig:1.xml</SCPDURL>
</service>
</serviceList>
<deviceList>
<device>
<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>
<friendlyName>urn:schemas-upnp-org:device:WANConnectionDevice:1</friendlyName>
<manufacturer>Remi Inc.</manufacturer>
<manufacturerURL>http://www.remi.com/</manufacturerURL>
<modelDescription>Internet Access Server</modelDescription>
<modelName>DD-WRT</modelName>
<modelNumber>DD-WRT v23 SP1 Final (05/16/06)</modelNumber>
<modelURL>http://www.remi.com/</modelURL>
<UDN>uuid:0012-1707-c2e502100c00</UDN>
<serviceList>
<service>
<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>
<controlURL>/uuid:0012-1707-c2e502100c00/WANIPConnection:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e502100c00/WANIPConnection:1</eventSubURL>
<SCPDURL>/dynsvc/WANIPConnection:1.xml</SCPDURL>
</service>
<service>
<serviceType>urn:schemas-upnp-org:service:WANPPPConnection:1</serviceType>
<serviceId>urn:upnp-org:serviceId:WANPPPConn1</serviceId>
<controlURL>/uuid:0012-1707-c2e502100c00/WANPPPConnection:1</controlURL>
<eventSubURL>/uuid:0012-1707-c2e502100c00/WANPPPConnection:1</eventSubURL>
<SCPDURL>/dynsvc/WANPPPConnection:1.xml</SCPDURL>
</service>
</serviceList>
</device>
</deviceList>
</device>
</deviceList>
</device>
</root>
'

res="$(echo $xml | ./test_device 2>&1 )" || fatal "$res"

diff -u -b -B - <(echo "$res" |egrep -v '(Discover|talloc)') <<EOF || fatal
  |
  +- UDN            = uuid:0012-1707-c2e500a80c00
  +- DeviceType     = urn:schemas-upnp-org:device:InternetGatewayDevice:1
  +- DescDocURL     = http://test.url
  +- FriendlyName   = My Router
  +- PresURL        = http://192.168.1.1/UPnP.asp

EOF


echo " OK"


exit 0


