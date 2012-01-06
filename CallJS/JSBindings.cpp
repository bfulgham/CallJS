/*
 * Copyright (C) 2011 Brent Fulgham.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "stdafx.h"

#include "CallJSDlg.h"

#include <WebKit/WebKit.h>
#include <WebKit/WebKitCOMAPI.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include <JavaScriptCore/JSStringRefBSTR.h>

#include <winsock2.h>
#include <assert.h>
#include <shlwapi.h>
#include <tchar.h>
#include <wininet.h>

static JSValueRef console_setscript(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef[] /*arguments*/, JSValueRef* /*exception*/);
static JSValueRef console_report(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef[] /*arguments*/, JSValueRef* /*exception*/);
static JSValueRef console_log(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t /*argumentCount*/, const JSValueRef[] /*arguments*/, JSValueRef* /*exception*/);

static JSValueRef console_getSharedValue(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef* /*exception*/);
static JSValueRef console_getSharedDouble(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef* /*exception*/);
static JSValueRef console_getSharedBool(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef* /*exception*/);

static bool console_setSharedValue(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef /*value*/, JSValueRef* /*exception*/);
static bool console_setSharedDouble(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef /*value*/, JSValueRef* /*exception*/);
static bool console_setSharedBool(JSContextRef ctx, JSObjectRef /*thisObject*/, JSStringRef /*propertyName*/, JSValueRef /*value*/, JSValueRef* /*exception*/);

/**
 * Convenience method (constructor) to build the console object.
 */
JSClassRef ConsoleClass()
{
    static JSStaticValue staticValues[] = {
        { "sharedValue", console_getSharedValue, console_setSharedValue, kJSPropertyAttributeNone },
        { "sharedDouble", console_getSharedDouble, console_setSharedDouble, kJSPropertyAttributeNone },
        { "sharedBool", console_getSharedBool, console_setSharedBool, kJSPropertyAttributeNone },
        { 0, 0, 0, 0 }
    };

    static JSStaticFunction staticFunctions[] = {
        { "setscript", console_setscript, kJSPropertyAttributeNone },
        { "report", console_report, kJSPropertyAttributeNone },
        { "log", console_log, kJSPropertyAttributeNone },
        { 0, 0, 0 }
    };

    JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "console", 0, staticValues, staticFunctions,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    static JSClassRef consoleClass = JSClassCreate(&classDefinition);
    return consoleClass;
}

/**
 * The callback from JavaScriptCore.  We told JSC to call this function
 * whenever it sees "console.setscript".
 */
static JSValueRef console_setscript(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef* arguments, JSValueRef* exception)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    if (argumentCount < 1)
        return JSValueMakeUndefined(ctx);

   // Convert the result into a string for display.
   if (!JSValueIsString(ctx, arguments[0]))
        return JSValueMakeUndefined(ctx);

   JSStringRef temp = JSValueToStringCopy (ctx, arguments[0], exception);
   if (exception && *exception)
      return JSValueMakeUndefined(ctx);

   BSTR strTemp = ::SysAllocString(JSStringGetCharactersPtr(temp));
   JSStringRelease(temp);

   CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
   dlg->setScript(strTemp);
   ::SysFreeString(strTemp);

   return JSValueMakeUndefined(ctx);
}

/**
 * The callback from JavaScriptCore.  We told JSC to call this function
 * whenever it sees "console.report".
 */
static JSValueRef console_report(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef* arguments, JSValueRef* exception)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return JSValueMakeUndefined(ctx);

    if (argumentCount)
        return JSValueMakeUndefined(ctx);

    JSStringRef jsString = JSStringCreateWithBSTR(dlg->sharedString());
    JSValueRef jsValue = JSValueMakeString(ctx, jsString);
    JSStringRelease(jsString);

    return jsValue;
}

static JSValueRef console_getSharedValue(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef* /*exception*/)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return JSValueMakeUndefined(ctx);

    JSStringRef jsString = JSStringCreateWithBSTR(dlg->sharedString());
    JSValueRef jsValue = JSValueMakeString(ctx, jsString);
    JSStringRelease(jsString);

    return jsValue;
}

static JSValueRef console_getSharedDouble(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef* /*exception*/)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return JSValueMakeUndefined(ctx);

    return JSValueMakeNumber(ctx, dlg->sharedDouble());
}

static JSValueRef console_getSharedBool(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef* /*exception*/)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return JSValueMakeUndefined(ctx);

    return JSValueMakeBoolean(ctx, dlg->sharedBool());
}

static bool console_setSharedValue(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef value, JSValueRef* exception)
{
    if (!JSValueIsString(ctx, value))
        return false;

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return false;

    JSStringRef temp = JSValueToStringCopy (ctx, value, exception);
    if (exception && *exception)
        return false;

    BSTR strTemp = ::SysAllocString(JSStringGetCharactersPtr(temp));
    JSStringRelease(temp);

    dlg->setSharedString(strTemp);
    ::SysFreeString(strTemp);

    return true;
}

static bool console_setSharedDouble(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef value, JSValueRef* exception)
{
    if (!JSValueIsNumber(ctx, value))
        return false;

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return false;

    double temp = JSValueToNumber (ctx, value, exception);
    if (exception && *exception)
        return false;

    dlg->setSharedDouble(temp);

    return true;
}

static bool console_setSharedBool(JSContextRef ctx, JSObjectRef thisObject, JSStringRef /*propertyName*/, JSValueRef value, JSValueRef* /*exception*/)
{
    if (!JSValueIsBoolean(ctx, value))
        return false;

    CallJSDlg* dlg = static_cast<CallJSDlg*>(JSObjectGetPrivate(thisObject));
    if (!dlg)
        return false;

    bool temp = JSValueToBoolean (ctx, value);
    dlg->setSharedBool(temp);

    return true;
}

/**
 * The callback from JavaScriptCore.  We told JSC to call this function
 * whenever it sees "console.log".
 */
static JSValueRef console_log(JSContextRef ctx, JSObjectRef /*function*/, JSObjectRef thisObject, size_t argumentCount, const JSValueRef* arguments, JSValueRef* exception)
{
    if (!JSValueIsObjectOfClass(ctx, thisObject, ConsoleClass()))
        return JSValueMakeUndefined(ctx);

    if (argumentCount < 1)
        return JSValueMakeUndefined(ctx);

   // Convert the result into a string for display.
   if (!JSValueIsString(ctx, arguments[0]))
        return JSValueMakeUndefined(ctx);

   JSStringRef temp = JSValueToStringCopy (ctx, arguments[0], exception);
   if (exception && *exception)
      return JSValueMakeUndefined(ctx);

   BSTR strTemp = ::SysAllocString(JSStringGetCharactersPtr(temp));
   JSStringRelease(temp);

   ::OutputDebugString(strTemp);
   ::OutputDebugString(_T("\r\n"));

   ::SysFreeString(strTemp);

   return JSValueMakeUndefined(ctx);
}
