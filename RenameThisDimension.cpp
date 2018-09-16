
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>


using namespace adsk::core;
using namespace adsk::fusion;

Ptr<Application> app;
Ptr<UserInterface> ui;

const std::string panelId = "SolidCreatePanel";

extern "C" XI_EXPORT bool run(const char* context)
{
	app = Application::get();
	if (!app)
		return false;

	ui = app->userInterface();
	if (!ui)
		return false;

	ui->messageBox("Hello addin");

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (ui)
	{
		ui->messageBox("Stop addin");
		ui = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
