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

#include <winsock2.h>
#include <assert.h>
#include <shlwapi.h>
#include <tchar.h>
#include <wininet.h>

#include "CallJS.h"
#include "CallJSDlg.h"
#include "JSBindings.h"

#include <WebKit/WebKit.h>
#include <WebKit/WebKitCOMAPI.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include <JavaScriptCore/JSStringRefBSTR.h>

CallJSDlg::CallJSDlg(HINSTANCE hInstance, int cmdShow)
	: m_dialog(0), m_hInstance(hInstance), m_mainWebView(0), m_mainWebFrame (0),
      m_refCount(0), m_sharedDouble(3.14159265), m_sharedBool(false), 
      m_partner(0), m_hudWebView (0), m_hudWnd(0), m_cmdShow(cmdShow)
{
    m_hIcon = ::LoadIcon(m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));

    m_sFirstParam = ::SysAllocString(_T("the first parameter value"));
    m_sSecondParam = ::SysAllocString(_T("and the second parameter value"));
    m_sCallResult = ::SysAllocString(_T(""));
    m_sJavaScript = ::SysAllocString(_T("'Hello JavaScript!'"));
    m_sRunResult = ::SysAllocString(_T(""));

    m_sharedString = ::SysAllocString(_T("okay"));
}

CallJSDlg::~CallJSDlg()
{
    ::SysFreeString(m_sFirstParam);
    ::SysFreeString(m_sSecondParam);
    ::SysFreeString(m_sCallResult);
    ::SysFreeString(m_sJavaScript);
    ::SysFreeString(m_sRunResult);

    ::SysFreeString(m_sharedString);

    if (m_mainWebFrame)
        m_mainWebFrame->Release();
    if (m_mainWebView)
        m_mainWebView->Release();
    if (m_hudWebView)
        m_hudWebView->Release();
}

void CallJSDlg::connectControls()
{
    ::SetDlgItemText(m_dialog, IDC_EDIT_P1, m_sFirstParam);
    ::SetDlgItemText(m_dialog, IDC_EDIT_P2, m_sSecondParam);
    ::SetDlgItemText(m_dialog, IDC_CALL_RESULT, m_sCallResult);
    ::SetDlgItemText(m_dialog, IDC_JAVASCRIPT_ENTRY, m_sJavaScript);
    ::SetDlgItemText(m_dialog, IDC_RUN_RESULT, m_sRunResult);
}

void CallJSDlg::Initialize()
{
    m_dialog = ::CreateDialog(m_hInstance, MAKEINTRESOURCE(IDD_CALLJS_DIALOG), 0, DialogProc);

    ::SetWindowLongPtr(m_dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(this));

    if (FAILED(WebKitCreateInstance(CLSID_WebView, 0, IID_IWebView, reinterpret_cast<void**>(&m_mainWebView))))
        return;

    HRESULT hr = m_mainWebView->setHostWindow(reinterpret_cast<OLE_HANDLE>(m_dialog));
    if (FAILED(hr))
        return;

    hr = m_mainWebView->setUIDelegate(this);
    if (FAILED(hr))
        return;

    HWND groupBox = ::GetDlgItem(m_dialog, IDC_WEBKIT_GROUP);

    RECT containerRect;
    ::GetWindowRect(groupBox, &containerRect);

    POINT corner = { containerRect.left, containerRect.top };
    ::ScreenToClient(m_dialog, &corner);

    int width = containerRect.right - containerRect.left - 10;
    int height = containerRect.bottom - containerRect.top - 25;

    // Shrink the box a bit to fit
    containerRect.left = corner.x + 5;
    containerRect.top = corner.y + 20;
    containerRect.right = containerRect.left + width;
    containerRect.bottom = containerRect.top + height;

    hr = m_mainWebView->initWithFrame(containerRect, 0, 0);
    if (FAILED(hr))
        return;

    hr = m_mainWebView->mainFrame(&m_mainWebFrame);
    if (FAILED(hr))
        return;

#if 0
    // Simple test to make sure things are hooked up properly
    static BSTR defaultHTML = 0;
    if (!defaultHTML)
        defaultHTML = SysAllocString(TEXT("<p style=\"background-color: #00FF00\">Testing</p><img src=\"http://webkit.org/images/icon-gold.png\" alt=\"Face\"><div style=\"border: solid blue\" contenteditable=\"true\">div with blue border</div><ul><li>foo<li>bar<li>baz</ul>"));

    m_mainWebFrame->loadHTMLString(defaultHTML, 0);
#else
    static BSTR urlBStr = 0;
    if (!urlBStr) {
        TCHAR szFileName[MAX_PATH];
        ::GetModuleFileName(NULL, szFileName, MAX_PATH);
        ::PathRemoveFileSpec(szFileName);
        ::PathAddBackslash(szFileName);
        ::PathAppend(szFileName, _T("test.html"));

        TCHAR szFileName2[MAX_PATH];
        _stprintf_s(szFileName2, MAX_PATH, _T("file://%s"), szFileName);
        urlBStr = SysAllocString(szFileName2);
    }

    if ((PathFileExists(urlBStr) || PathIsUNC(urlBStr))) {
        TCHAR fileURL[INTERNET_MAX_URL_LENGTH];
        DWORD fileURLLength = sizeof(fileURL)/sizeof(fileURL[0]);
        if (SUCCEEDED(UrlCreateFromPath(urlBStr, fileURL, &fileURLLength, 0)))
            urlBStr = fileURL;
    }

    IWebMutableURLRequest* request = 0;
    if (FAILED(WebKitCreateInstance(CLSID_WebMutableURLRequest, 0, IID_IWebMutableURLRequest, (void**)&request)))
        return;

    hr = request->initWithURL(urlBStr, WebURLRequestUseProtocolCachePolicy, 60);
    if (FAILED(hr))
        return;

    hr = m_mainWebFrame->loadRequest(request);
    if (FAILED(hr))
        return;

    if (request)
        request->Release();
#endif

    connectControls ();

    ::ShowWindow(m_dialog, m_cmdShow);

    makeScriptObject();

    CreateHUD();
}

void CallJSDlg::CreateHUD()
{
    RECT clientRect = clientRectForHUD();

    IWebViewPrivate* webViewPrivate = 0;
    IWebMutableURLRequest* request = 0;
    IWebPreferences* tmpPreferences = 0;
    IWebPreferences* standardPreferences = 0;
    if (FAILED(WebKitCreateInstance(CLSID_WebPreferences, 0, IID_IWebPreferences, reinterpret_cast<void**>(&tmpPreferences))))
        goto exit;

    if (FAILED(tmpPreferences->standardPreferences(&standardPreferences)))
        goto exit;

    standardPreferences->setAcceleratedCompositingEnabled(TRUE);

    HRESULT hr = WebKitCreateInstance(CLSID_WebView, 0, IID_IWebView, reinterpret_cast<void**>(&m_hudWebView));
    if (FAILED(hr))
        goto exit;

    hr = m_hudWebView->QueryInterface(IID_IWebViewPrivate, reinterpret_cast<void**>(&webViewPrivate));
    if (FAILED(hr))
        goto exit;

    hr = m_hudWebView->initWithFrame(clientRect, 0, 0);
    if (FAILED(hr))
        goto exit;

    IWebFrame* frame;
    hr = m_hudWebView->mainFrame(&frame);
    if (FAILED(hr))
        goto exit;

#if 0
    static BSTR defaultHTML = SysAllocString(TEXT("<p style=\"background-color: #00FF00\">Testing</p><img src=\"http://webkit.org/images/icon-gold.png\" alt=\"Face\"><div style=\"border: solid blue; background: white;\" contenteditable=\"true\">div with blue border</div><ul><li>foo<li>bar<li>baz</ul>"));
    frame->loadHTMLString(defaultHTML, 0);
    frame->Release();
#else
    static BSTR urlBStr = 0;
    if (!urlBStr) {
        TCHAR szFileName[MAX_PATH];
        ::GetModuleFileName(NULL, szFileName, MAX_PATH);
        ::PathRemoveFileSpec(szFileName);
        ::PathAddBackslash(szFileName);
        ::PathAppend(szFileName, _T("overlay.html"));

        TCHAR szFileName2[MAX_PATH];
        _stprintf_s(szFileName2, MAX_PATH, _T("file://%s"), szFileName);
        urlBStr = SysAllocString(szFileName2);
    }

    if ((PathFileExists(urlBStr) || PathIsUNC(urlBStr))) {
        TCHAR fileURL[INTERNET_MAX_URL_LENGTH];
        DWORD fileURLLength = sizeof(fileURL)/sizeof(fileURL[0]);
        if (SUCCEEDED(UrlCreateFromPath(urlBStr, fileURL, &fileURLLength, 0)))
            urlBStr = fileURL;
    }

    if (FAILED(WebKitCreateInstance(CLSID_WebMutableURLRequest, 0, IID_IWebMutableURLRequest, (void**)&request)))
        goto exit;

    hr = request->initWithURL(urlBStr, WebURLRequestUseProtocolCachePolicy, 60);
    if (FAILED(hr))
        goto exit;

    hr = frame->loadRequest(request);
    if (FAILED(hr))
        goto exit;

    if (request)
        request->Release();
    frame->Release();
#endif

    hr = webViewPrivate->setTransparent(TRUE);
    if (FAILED(hr))
        goto exit;

    hr = webViewPrivate->setUsesLayeredWindow(TRUE);
    if (FAILED(hr))
        goto exit;

    hr = webViewPrivate->viewWindow(reinterpret_cast<OLE_HANDLE*>(&m_hudWnd));
    if (FAILED(hr) || !m_hudWnd)
        goto exit;

    ::ShowWindow(m_hudWnd, m_cmdShow);
    ::UpdateWindow(m_hudWnd);

    // Put our two partner windows (the dialog and the HUD) in proper Z-order
    ::SetWindowPos(m_dialog, m_hudWnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

exit:
    if (standardPreferences)
        standardPreferences->Release();
    tmpPreferences->Release();
    if (webViewPrivate)
        webViewPrivate->Release();
}

RECT CallJSDlg::clientRectForHUD()
{
    RECT rect;
    ::GetClientRect(m_dialog, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    POINT pos = {rect.left, rect.top};
    ::ClientToScreen(m_dialog, &pos);

    rect.left = pos.x;
    rect.top = pos.y;
    rect.right = pos.x + width;
    rect.bottom = pos.y + height;

    return rect;
}

void CallJSDlg::moveHUD(int x, int y)
{
    RECT rect;
    ::GetWindowRect(m_hudWnd, &rect);
    ::MoveWindow(m_hudWnd, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

void CallJSDlg::resizeHUD(int width, int height)
{
    RECT rect;
    ::GetWindowRect(m_hudWnd, &rect);
    ::MoveWindow(m_hudWnd, rect.left, rect.top, width, height, TRUE);
}

void CallJSDlg::setScript(BSTR script)
{
    ::SysFreeString(m_sJavaScript);
    m_sJavaScript = ::SysAllocString(script);

    ::SetDlgItemText(m_dialog, IDC_JAVASCRIPT_ENTRY, m_sJavaScript);
}

void CallJSDlg::setSharedString(BSTR newString)
{
    ::SysFreeString(m_sharedString);
    m_sharedString = ::SysAllocString(newString);
}


/**
 * This method is called when the 'Run JavaScript' button is pressed in the UI.
 * Here, we retrieve the JavaScript we want to run from the UI, run the script, and
 * display the results in the UI.
 */
void CallJSDlg::runJavascript()
{
    TCHAR buffer[4096];
    ::GetDlgItemText(m_dialog, IDC_JAVASCRIPT_ENTRY, buffer, sizeof(buffer));
    ::SysReAllocString(&m_sJavaScript, buffer);

    BSTR result;
    HRESULT hr = m_mainWebView->stringByEvaluatingJavaScriptFromString(m_sJavaScript, &result);
    if (FAILED(hr))
    {
        OutputDebugString(_T("WebKit: stringByEvaluatingJavaScriptFromString failed.\n"));
        OutputDebugString(m_sJavaScript);
        OutputDebugString(_T("\n"));
        ::SysReAllocString(&result, _T("<<FAILURE>>"));
    }

    ::SysFreeString(m_sRunResult);
    m_sRunResult = result;

    ::SetDlgItemText(m_dialog, IDC_RUN_RESULT, m_sRunResult);
}

/**
 * This method is called when the 'Call JavaScript Function' button is pressed
 * in the UI.  Here, we retrieve the parameters from the two text fields in the UI,
 * build a parameter list, and then we call through to the JavaScript function named
 * 'SampleFunction'.  The result returned by 'SampleFunction' is displayed in the UI.
 */
void CallJSDlg::callJsFunction()
{
   JSGlobalContextRef runCtx = m_mainWebFrame->globalContext();

   static JSStringRef methodName = 0;
   if (!methodName)
      methodName = JSStringCreateWithUTF8CString("SampleFunction");

   JSValueRef args[2];
   args[0] = JSValueMakeString (runCtx, JSStringCreateWithCharacters((LPCTSTR)m_sFirstParam, ::SysStringLen(m_sFirstParam)));
   args[1] = JSValueMakeString (runCtx, JSStringCreateWithCharacters((LPCTSTR)m_sSecondParam, ::SysStringLen(m_sSecondParam)));

   JSObjectRef global = JSContextGetGlobalObject(runCtx);

   JSValueRef* exception = 0;
   JSValueRef sampleFunction = JSObjectGetProperty(runCtx, global, methodName, exception);
   if (exception)
      return;

   assert(JSValueIsObject(runCtx, sampleFunction));

   JSObjectRef function = JSValueToObject(runCtx, sampleFunction, exception);
   if (exception)
      return;

   JSValueRef result = JSObjectCallAsFunction (runCtx, function, global, 2, args, exception);
   if (exception)
      return;

   // Convert the result into a string for display.
   if (JSValueIsString(runCtx, result))
   {
      JSStringRef temp = JSValueToStringCopy (runCtx, result, exception);
      if (exception)
         return;

      ::SysReAllocString(&m_sCallResult, JSStringGetCharactersPtr(temp));
      JSStringRelease(temp);
   }
   else if (JSValueIsNumber(runCtx, result))
   {
      double temp = JSValueToNumber(runCtx, result, exception);
      if (exception)
         return;

      TCHAR buffer[1024];
      _stprintf_s(buffer, 1024, _T("%g"), temp);
      ::SysReAllocString(&m_sCallResult, buffer);
   }
   else if (JSValueIsBoolean(runCtx, result))
   {
      bool temp = JSValueToBoolean(runCtx, result);
      if (exception)
         return;

      ::SysReAllocString(&m_sCallResult, ((temp) ? _T("true") : _T("false")));
   }

   ::SetDlgItemText(m_dialog, IDC_CALL_RESULT, m_sCallResult);
}

/**
 * Create a JavaScript object named 'console' that will be JavaScriptCore's
 * mechanism for making updates to this program's interface.
 */
void CallJSDlg::makeScriptObject()
{
   if (!m_mainWebFrame)
      return;

   JSGlobalContextRef runCtx = m_mainWebFrame->globalContext();

   JSObjectRef global = JSContextGetGlobalObject(runCtx);
   assert(global);

   JSClassRef consoleClass = ConsoleClass();
   assert(consoleClass);

   m_scriptObject = JSObjectMake(runCtx, consoleClass, reinterpret_cast<void*>(this));
   assert(m_scriptObject);

   JSStringRef consoleName = JSStringCreateWithUTF8CString("console");
   JSObjectSetProperty(runCtx, global, consoleName, m_scriptObject, kJSPropertyAttributeNone, 0);
   JSStringRelease(consoleName);
}

void CallJSDlg::readTextField(int fieldID)
{
    TCHAR buffer[4096];
    ::GetDlgItemText(m_dialog, fieldID, buffer, sizeof(buffer));

    BSTR* field = 0;

    switch (fieldID) {
    case IDC_EDIT_P1:
        field = &m_sFirstParam;
        break;
    case IDC_EDIT_P2:
        field = &m_sSecondParam;
        break;
    case IDC_CALL_RESULT:
        field = &m_sCallResult;
        break;
    case IDC_JAVASCRIPT_ENTRY:
        field = &m_sJavaScript;
        break;
    case IDC_RUN_RESULT:
        field = &m_sRunResult;
        break;
    default:
        break;
    }

    ::SysReAllocString(field, buffer);
}

BOOL CallJSDlg::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CallJSDlg* dialog = reinterpret_cast<CallJSDlg*>(::GetWindowLongPtr(hDlg, DWLP_USER));
    if (!dialog)
        return FALSE;

    switch (message) {
    case WM_CLOSE:
        ::PostQuitMessage(0);
        return FALSE;
    case WM_INITDIALOG:
        dialog->connectControls();
        break;
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* p = reinterpret_cast<WINDOWPOS*>(lParam);
        p->hwndInsertAfter = dialog->getPartnerWnd();
        p->flags &= ~SWP_NOZORDER;
        return TRUE;
        }
        break;
    case WM_MOVE: {
            int x = (int)(short) LOWORD(lParam);   // horizontal position 
            int y = (int)(short) HIWORD(lParam);
            dialog->moveHUD(x, y);
            return FALSE;
        }
        break;
    case WM_SIZE: {
            int width = (int)(short) LOWORD(lParam);   // horizontal position 
            int height = (int)(short) HIWORD(lParam);
            dialog->resizeHUD(width, height);
            return FALSE;
        }
        break;
    case WM_COMMAND:
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        switch (wmId) {
        case IDC_EDIT_P1:
        case IDC_EDIT_P2:
        case IDC_CALL_RESULT:
        case IDC_JAVASCRIPT_ENTRY:
        case IDC_RUN_RESULT:
            if (EN_KILLFOCUS == wmEvent || EN_CHANGE == wmEvent)
                dialog->readTextField(wmId);
            break;
        case IDC_CALL_JS_FUNCTION:
            dialog->callJsFunction();
            break;
        case IDC_RUN_JAVASCRIPT:
            dialog->runJavascript();
            break;
        case IDM_ABOUT:
            //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            //DestroyWindow(hWnd);
            break;
        case IDM_PRINT:
            //PrintView(hWnd, message, wParam, lParam);
            break;
        default:
            return FALSE;
        }
        break;
    }

    return FALSE;
}

HRESULT CallJSDlg::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebUIDelegate*>(this);
    else if (IsEqualIID(riid, IID_IWebUIDelegate))
        *ppvObject = static_cast<IWebUIDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG CallJSDlg::AddRef(void)
{
    return ++m_refCount;
}

ULONG CallJSDlg::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT CallJSDlg::runJavaScriptAlertPanelWithMessage(IWebView* sender, BSTR message)
{
    ::MessageBox(m_dialog, message, _T("WebKit Alert!"), MB_OK);
    return S_OK;
}
