# CallJS

This repository contains the source code for CallJS, an example program showing off some of the features available in the WinCairo port of [WebKit][].

# Building

You should be able to simply open the `CallJS.sln` solution and build it.

Please note that you need to make sure that you have the correct Visual Studio runtime libraries to go along with the pre-built WebKit.dll and JavaScriptCore.dll in the 'WebKitSDK' folder.

# Examples
## Snowfall Overlay

![CallJS](https://raw.github.com/bfulgham/CallJS/gh-pages/snowfall_effect.png)

The current source build runs with a transparent WebView overlaying the entire screen.  It plays a snowfall animation while you interact with the fields and buttons on the application.

[WebKit]:	http://webkit.org