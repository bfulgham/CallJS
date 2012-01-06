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

#include "CallJS.h"

#include "CallJSDlg.h"

#include <WebKit/WebKitCOMAPI.h>

#include <string>

#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <tchar.h>
#include <wininet.h>


CallJS::CallJS(HINSTANCE hInstance, MSG& msg, int cmdShow)
      :m_hInstance(hInstance), m_msg(msg),
       m_dialog(0), m_cmdShow(cmdShow), m_windowTitle(L"CallJS Application")
{
}

CallJS::~CallJS()
{
}

void CallJS::Initialize()
{
    INITCOMMONCONTROLSEX InitCtrlEx;

    InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCtrlEx.dwICC  = 0x00004000; //ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&InitCtrlEx);

    OleInitialize(0);

    CreateMainWindow();
}

void CallJS::CreateMainWindow()
{
    m_dialog = new CallJSDlg(m_hInstance, m_cmdShow);
    m_dialog->setPartner(this);
    m_dialog->Initialize();
}

void CallJS::Run()
{
    HACCEL hAccelTable = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDC_CALLJS));

    // Main message loop:
    while (GetMessage(&m_msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(m_msg.hwnd, hAccelTable, &m_msg)) {
            TranslateMessage(&m_msg);
            DispatchMessage(&m_msg);
        }
    }
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine,
                       int nCmdShow)
{
#ifdef _CRTDBG_MAP_ALLOC
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);

    MSG msg = {0};

    CallJS theApp(hInstance, msg, nCmdShow);
    theApp.Initialize();
    theApp.Run();

    shutDownWebKit();
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif

    // Shut down COM.
    OleUninitialize();
    
    return static_cast<int>(msg.wParam);
}
