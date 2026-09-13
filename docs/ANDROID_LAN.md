# Android LAN notes for the native edition

The browser/PWA edition uses WebRTC and does not need raw LAN socket permissions. This document applies to the native Cocos2d-x Android edition.

The C++ UDP transport uses BSD sockets on Android through the non-Windows path in `Classes/Network/NetworkManager.cpp`. Android app packaging must also declare/request the platform's local-network permissions.

For current Android targets:

- keep `android.permission.INTERNET`
- declare/request `android.permission.NEARBY_WIFI_DEVICES` where nearby Wi-Fi APIs or current local-network protections require it
- Android 17 / API 37 introduces the dedicated runtime `android.permission.ACCESS_LOCAL_NETWORK` permission for apps targeting API 37+

LAN test cases should cover UDP broadcast discovery, incoming/outgoing UDP unicast, denied permission, Wi-Fi client isolation, VPN routing, and reconnect after app background/foreground.

The PWA does not use this permission path; WebRTC is the supported Android browser transport.
