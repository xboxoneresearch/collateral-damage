// Send toast notifications in Windows 10, using Windows Runtime,
// without any language projection, in PLAIN C
// Copyright (c) 2021 Valentin - Gabriel Radu
//
// MIT License
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Due to the long struct names and because I don't want to abstractize and 
// typedef structs which has the potential to obfuscate the code, I decided to
// stick to 120 character lines for this document, instead of 80
//
// Set project Properties - Configuration Properties - Linker - All Options -
// - Additional Dependencies - runtimeobject.lib
// #pragma comment(lib, "runtimeobject.lib") does not work when compiled 
// without default lib for whatever reason
//
// Set project Properties - Configuration Properties - Linker - All Options - 
// - SubSystem - Windows or change to Console
// and modify the entry point and the signature of "main"
// the pragma belowis optional
//
// This code is basically the example provided by Microsoft without all the 
// bloat around it and written in plain C instead of C++, of course
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/DesktopToasts/CPP/DesktopToastsSample.cpp
//
// Also, I have taken out the COM activator part; maybe I will port over that
// in the future, but for my use case, I don't really need it; the 
// notifications still persist in Action Center and I fire my app with a 
// protocol; this has the advantage that you DO NOT have to install a shortcut 
// in the Start menu and can use the shortcut of any application that has a
// shortcut in Start with an AppUserModelId; this is great if you do a plugin 
// for an app, so you do not really need a shortcut of your own to clutter the
// Start menu. 
// Also, the COM activator is required for buttons on the toast.
// To see the IDs for installed apps, run "Get-StartApps" in PowerShell.
//
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <initguid.h>
#include <roapi.h>
#include <Windows.ui.notifications.h>
#include <windows.ui.popups.h>

#define Done(code) ExitProcess(code)

/* Toasts */

// UUIDs obtained from <windows.ui.notifications.h>
//
// ABI.Windows.UI.Notifications.IToastNotificationManagerStatics
// 50ac103f-d235-4598-bbef-98fe4d1a3ad4
DEFINE_GUID(UIID_IToastNotificationManagerStatics,
	0x50ac103f,
	0xd235, 0x4598, 0xbb, 0xef,
	0x98, 0xfe, 0x4d, 0x1a, 0x3a, 0xd4
);
//
// ABI.Windows.Notifications.IToastNotificationFactory
// 04124b20-82c6-4229-b109-fd9ed4662b53
DEFINE_GUID(UIID_IToastNotificationFactory,
	0x04124b20,
	0x82c6, 0x4229, 0xb1, 0x09,
	0xfd, 0x9e, 0xd4, 0x66, 0x2b, 0x53
);

// UUIDs obtained from <windows.data.xml.dom.h>
//
// ABI.Windows.Data.Xml.Dom.IXmlDocument
// f7f3a506-1e87-42d6-bcfb-b8c809fa5494
DEFINE_GUID(UIID_IXmlDocument,
	0xf7f3a506,
	0x1e87, 0x42d6, 0xbc, 0xfb,
	0xb8, 0xc8, 0x09, 0xfa, 0x54, 0x94
);
//
// ABI.Windows.Data.Xml.Dom.IXmlDocumentIO
// 6cd0e74e-ee65-4489-9ebf-ca43e87ba637
DEFINE_GUID(UIID_IXmlDocumentIO,
	0x6cd0e74e,
	0xee65, 0x4489, 0x9e, 0xbf,
	0xca, 0x43, 0xe8, 0x7b, 0xa6, 0x37
);

/* MessageDialog */

// UUIDs obtained from <windows.ui.popups.h>
//
// ABI.Windows.UI.Popups.IMessageDialog
// 33F59B01-5325-43AB-9AB3-BDAE440E4121
DEFINE_GUID(UIID_IMessageDialog,
	0x33f59b01,
	0x5325, 0x43ab, 0x9a, 0xb3,
	0xbd, 0xae, 0x44, 0x0e, 0x41, 0x21
);

//
// ABI.Windows.UI.Popups.IMessageDialogFactory
// 2D161777-A66F-4EA5-BB87-793FFA4941F2
DEFINE_GUID(UIID_IMessageDialogFactory,
	0x2d161777,
	0xa66f, 0x4ea5, 0xbb, 0x87,
	0x79, 0x3f, 0xfa, 0x49, 0x41, 0xf2
);

//
// ABI.Windows.UI.Popups.IUICommand
// 4FF93A75-4145-47FF-AC7F-DFF1C1FA5B0F
DEFINE_GUID(UIID_IUICommand,
	0x4ff93a75,
	0x4145, 0x47ff, 0xac, 0x7f,
	0xdf, 0xf1, 0xc1, 0xfa, 0x5b, 0x0f
);
//
// ABI.Windows.UI.Popups.IUICommandFactory
// A21A8189-26B0-4676-AE94-54041BC125E8
DEFINE_GUID(UIID_IUICommandFactory,
	0xa21a8189,
	0x26b0, 0x4676, 0xae, 0x94,
	0x54, 0x04, 0x1b, 0xc1, 0x25, 0xe8
);

#define APP_ID L"XboxOneSystemToasts!Windows.Xbox.SystemToasts.Achievements"

HRESULT create_xml_document_from_string(
	const wchar_t* xmlString,
	__x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument** doc
)
{
	HRESULT hr = S_OK;

	HSTRING_HEADER header_IXmlDocumentHString;
	HSTRING IXmlDocumentHString;
	hr = WindowsCreateStringReference(
		RuntimeClass_Windows_Data_Xml_Dom_XmlDocument,
		(UINT32)wcslen(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument),
		&header_IXmlDocumentHString,
		&IXmlDocumentHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		return hr;
	}
	if (IXmlDocumentHString == NULL)
	{
		return E_POINTER;
	}

	IInspectable* pInspectable;
	hr = RoActivateInstance(IXmlDocumentHString, &pInspectable);
	if (SUCCEEDED(hr))
	{
		hr = pInspectable->lpVtbl->QueryInterface(
			pInspectable,
			&UIID_IXmlDocument,
			doc
		);
		pInspectable->lpVtbl->Release(pInspectable);
	}
	else
	{
		printf("%s:%d:: RoActivateInstance\n", __FUNCTION__, __LINE__);
		return hr;
	}

	__x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO* docIO;
	(*doc)->lpVtbl->QueryInterface(
		(*doc),
		&UIID_IXmlDocumentIO,
		&docIO
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: QueryInterface\n", __FUNCTION__, __LINE__);
		return hr;
	}

	HSTRING_HEADER header_XmlString;
	HSTRING XmlString;
	hr = WindowsCreateStringReference(
		xmlString,
		(UINT32)wcslen(xmlString),
		&header_XmlString,
		&XmlString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		docIO->lpVtbl->Release(docIO);
		return hr;
	}
	if (XmlString == NULL)
	{
		return E_POINTER;
	}

	hr = docIO->lpVtbl->LoadXml(docIO, XmlString);

	docIO->lpVtbl->Release(docIO);

	return hr;
}

int WINAPI show_toast()
{
	HRESULT hr = S_OK;

	hr = RoInitialize(RO_INIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoInitialize\n", __FUNCTION__, __LINE__);
		goto exit0;
	}

	HSTRING_HEADER header_AppIdHString;
	HSTRING AppIdHString;
	hr = WindowsCreateStringReference(
		APP_ID,
		(UINT32)wcslen(APP_ID),
		&header_AppIdHString,
		&AppIdHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit1;
	}
	if (AppIdHString == NULL)
	{
		hr = E_POINTER;
		goto exit1;
	}

	__x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument* inputXml = NULL;
	hr = create_xml_document_from_string(
		L"<toast scenario=\'rareAchievement\'>\r\n"
		L"	<visual>\r\n"
		L"		<binding template=\"ToastGeneric\">\r\n"
		L"			<text>Collateral Damage</text>\r\n"
		L"			<text>achieved</text>\r\n"
		L"			<text>Enjoy!</text>\r\n"
		L"		</binding>\r\n"
		L"	</visual>\r\n"
		L"</toast>\r\n"
		, &inputXml
	);

	if (FAILED(hr))
	{
		printf("%s:%d:: CreateXmlDocumentFromString\n", __FUNCTION__, __LINE__);
		goto exit1;
	}

	HSTRING_HEADER header_ToastNotificationManagerHString;
	HSTRING ToastNotificationManagerHString;
	hr = WindowsCreateStringReference(
		RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
		(UINT32)wcslen(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager),
		&header_ToastNotificationManagerHString,
		&ToastNotificationManagerHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit2;
	}
	if (ToastNotificationManagerHString == NULL)
	{
		printf("%s:%d:: ToastNotificationManagerHString == NULL\n", __FUNCTION__, __LINE__);
		hr = E_POINTER;
		goto exit2;
	}

	__x_ABI_CWindows_CUI_CNotifications_CIToastNotificationManagerStatics* toastStatics = NULL;
	hr = RoGetActivationFactory(
		ToastNotificationManagerHString,
		&UIID_IToastNotificationManagerStatics,
		(LPVOID*)&toastStatics
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoGetActivationFactory\n", __FUNCTION__, __LINE__);
		goto exit2;
	}

	__x_ABI_CWindows_CUI_CNotifications_CIToastNotifier* notifier;
	hr = toastStatics->lpVtbl->CreateToastNotifierWithId(
		toastStatics,
		AppIdHString,
		&notifier
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: CreateToastNotifierWithId\n", __FUNCTION__, __LINE__);
		goto exit3;
	}

	HSTRING_HEADER header_ToastNotificationHString;
	HSTRING ToastNotificationHString;
	hr = WindowsCreateStringReference(
		RuntimeClass_Windows_UI_Notifications_ToastNotification,
		(UINT32)wcslen(RuntimeClass_Windows_UI_Notifications_ToastNotification),
		&header_ToastNotificationHString,
		&ToastNotificationHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit4;
	}
	if (ToastNotificationHString == NULL)
	{
		printf("%s:%d:: ToastNotificationHString == NULL\n", __FUNCTION__, __LINE__);
		hr = E_POINTER;
		goto exit4;
	}

	__x_ABI_CWindows_CUI_CNotifications_CIToastNotificationFactory* notifFactory = NULL;
	hr = RoGetActivationFactory(
		ToastNotificationHString,
		&UIID_IToastNotificationFactory,
		(LPVOID*)&notifFactory
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoGetActivationFactory\n", __FUNCTION__, __LINE__);
		goto exit4;
	}

	__x_ABI_CWindows_CUI_CNotifications_CIToastNotification* toast = NULL;
	hr = notifFactory->lpVtbl->CreateToastNotification(notifFactory, inputXml, &toast);
	if (FAILED(hr))
	{
		printf("%s:%d:: CreateToastNotification\n", __FUNCTION__, __LINE__);
		goto exit5;
	}

	hr = notifier->lpVtbl->Show(notifier, toast);
	if (FAILED(hr))
	{
		printf("%s:%d:: Show\n", __FUNCTION__, __LINE__);
		goto exit6;
	}

	// We have to wait a bit for the COM threads to deliver the notification
	// to the system, I think
	// Don't know any better, yielding (Sleep(0)) is not enough
	Sleep(1);

exit6:
	toast->lpVtbl->Release(toast);
exit5:
	notifFactory->lpVtbl->Release(notifFactory);
exit4:
	notifier->lpVtbl->Release(notifier);
exit3:
	toastStatics->lpVtbl->Release(toastStatics);
exit2:
	inputXml->lpVtbl->Release(inputXml);
exit1:
	RoUninitialize();
exit0:
	Done(hr);
}

int WINAPI show_message_dialog(PCWSTR dialogTitle, PCWSTR dialogContent)
{
	HRESULT hr = S_OK;

	hr = RoInitialize(RO_INIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoInitialize\n", __FUNCTION__, __LINE__);
		goto exit0;
	}

	HSTRING_HEADER header_DialogTitleHString;
	HSTRING DialogTitleHString;
	hr = WindowsCreateStringReference(
		dialogTitle,
		(UINT32)wcslen(dialogTitle),
		&header_DialogTitleHString,
		&DialogTitleHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit1;
	}
	if (DialogTitleHString == NULL)
	{
		hr = E_POINTER;
		goto exit1;
	}

	HSTRING_HEADER header_DialogContentHString;
	HSTRING DialogContentHString;
	hr = WindowsCreateStringReference(
		dialogContent,
		(UINT32)wcslen(dialogContent),
		&header_DialogContentHString,
		&DialogContentHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit1;
	}
	if (DialogContentHString == NULL)
	{
		hr = E_POINTER;
		goto exit1;
	}

	/* IMessageDialogFactory */
	HSTRING_HEADER header_MessageDialogFactoryHString;
	HSTRING MessageDialogFactoryHString;
	hr = WindowsCreateStringReference(
		RuntimeClass_Windows_UI_Popups_MessageDialog,
		(UINT32)wcslen(RuntimeClass_Windows_UI_Popups_MessageDialog),
		&header_MessageDialogFactoryHString,
		&MessageDialogFactoryHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit1;
	}
	if (MessageDialogFactoryHString == NULL)
	{
		printf("%s:%d:: MessageDialogFactoryHString == NULL\n", __FUNCTION__, __LINE__);
		hr = E_POINTER;
		goto exit1;
	}

	__x_ABI_CWindows_CUI_CPopups_CIMessageDialogFactory* msgDialogFactory = NULL;
	hr = RoGetActivationFactory(
		MessageDialogFactoryHString,
		&UIID_IMessageDialogFactory,
		(LPVOID*)&msgDialogFactory
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoGetActivationFactory\n", __FUNCTION__, __LINE__);
		goto exit1;
	}

	/* IUICommandFactory*/
	HSTRING_HEADER header_UICommandFactoryHString;
	HSTRING UICommandFactoryHString;
	hr = WindowsCreateStringReference(
		RuntimeClass_Windows_UI_Popups_UICommand,
		(UINT32)wcslen(RuntimeClass_Windows_UI_Popups_UICommand),
		&header_UICommandFactoryHString,
		&UICommandFactoryHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit2;
	}
	if (UICommandFactoryHString == NULL)
	{
		printf("%s:%d:: MessageDialogFactoryHString == NULL\n", __FUNCTION__, __LINE__);
		hr = E_POINTER;
		goto exit2;
	}

	__x_ABI_CWindows_CUI_CPopups_CIUICommandFactory* uiCommandFactory = NULL;
	hr = RoGetActivationFactory(
		UICommandFactoryHString,
		&UIID_IUICommandFactory,
		(LPVOID*)&uiCommandFactory
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: RoGetActivationFactory\n", __FUNCTION__, __LINE__);
		goto exit2;
	}

	__x_ABI_CWindows_CUI_CPopups_CIMessageDialog* messageDialog;
	hr = msgDialogFactory->lpVtbl->CreateWithTitle(
		msgDialogFactory,
		dialogContent,
		dialogTitle,
		&messageDialog
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: CreateWithTitle\n", __FUNCTION__, __LINE__);
		goto exit3;
	}

	__FIVector_1_Windows__CUI__CPopups__CIUICommand* commandsVec;
	hr = messageDialog->lpVtbl->get_Commands(messageDialog, &commandsVec);
	if (FAILED(hr))
	{
		printf("%s:%d:: get_Commands\n", __FUNCTION__, __LINE__);
		goto exit4;
	}

	HSTRING_HEADER header_RebootCommandHString;
	HSTRING RebootCommandHString;
	hr = WindowsCreateStringReference(
		L"Reboot",
		(UINT32)wcslen(L"Reboot"),
		&header_RebootCommandHString,
		&RebootCommandHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit5;
	}
	if (RebootCommandHString == NULL)
	{
		hr = E_POINTER;
		goto exit5;
	}

	__x_ABI_CWindows_CUI_CPopups_CIUICommand* rebootCommand;
	uiCommandFactory->lpVtbl->Create(uiCommandFactory, RebootCommandHString, &rebootCommand);
	if (FAILED(hr))
	{
		printf("%s:%d:: uiCommandFactory->Create (reboot)\n", __FUNCTION__, __LINE__);
		goto exit5;
	}

	HSTRING_HEADER header_ContinueCommandHString;
	HSTRING ContinueCommandHString;
	hr = WindowsCreateStringReference(
		L"Continue",
		(UINT32)wcslen(L"Continue"),
		&header_ContinueCommandHString,
		&ContinueCommandHString
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: WindowsCreateStringReference\n", __FUNCTION__, __LINE__);
		goto exit6;
	}
	if (ContinueCommandHString == NULL)
	{
		hr = E_POINTER;
		goto exit6;
	}

	__x_ABI_CWindows_CUI_CPopups_CIUICommand* continueCommand;
	uiCommandFactory->lpVtbl->Create(uiCommandFactory, ContinueCommandHString, &continueCommand);
	if (FAILED(hr))
	{
		printf("%s:%d:: uiCommandFactory->Create (continue)\n", __FUNCTION__, __LINE__);
		goto exit6;
	}

	commandsVec->lpVtbl->Append(commandsVec, rebootCommand);
	commandsVec->lpVtbl->Append(commandsVec, continueCommand);

	__FIAsyncOperation_1_Windows__CUI__CPopups__CIUICommand* uiCommand;
	hr = messageDialog->lpVtbl->ShowAsync(
		messageDialog,
		&uiCommand
	);
	if (FAILED(hr))
	{
		printf("%s:%d:: ShowAsync\n", __FUNCTION__, __LINE__);
		goto exit7;
	}

	__x_ABI_CWindows_CUI_CPopups_CIUICommand* chosenCommand;
	hr = uiCommand->lpVtbl->GetResults(uiCommand, &chosenCommand);
	if (FAILED(hr))
	{
		printf("%s:%d:: GetResults\n", __FUNCTION__, __LINE__);
		goto exit8;
	}


	BOOL reboot_requested = (chosenCommand == rebootCommand);

	// TODO: Do reboot

	chosenCommand->lpVtbl->Release(chosenCommand);
	
	exit8:
		uiCommand->lpVtbl->Release(uiCommand);
	exit7:
		continueCommand->lpVtbl->Release(continueCommand);
	exit6:
		rebootCommand->lpVtbl->Release(rebootCommand);
	exit5:
		commandsVec->lpVtbl->Release(commandsVec);
	exit4:
		messageDialog->lpVtbl->Release(messageDialog);
	exit3:
		uiCommandFactory->lpVtbl->Release(uiCommandFactory);
	exit2:
		msgDialogFactory->lpVtbl->Release(msgDialogFactory);
	exit1:
		RoUninitialize();
	exit0:
		Done(hr);
}