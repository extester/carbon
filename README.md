
#### About Carbon Framework

Carbon is a C++ embedded application framework and component library for easing common tasks used
in the embedded development. The component library contains:

- Template classes to start deveopment with;
- GPIO support classes;
- Modem support classes;
- TCP and UDP networking;
- DNS and NTP Client classes;
- JSON and XML support;
- HTTP protocol;
- RTP/RTSP/RTCP/SDP protocols;

#### Compiling carbon

Copy config.mak.in to config.mak and run make.

If you have the problems compiling jansson package:
- Make sure you have installed autoconf and libtool packages;
- Cd to thparty/jansson and run autoreconf -f -i

#### Learning Carbon

Framework contains a full set of example code and documentation (in russian so far).

#### License

The Carbon Framework is licensed under the apache 2.0 license.

