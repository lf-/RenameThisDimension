
#include <sstream>

#pragma warning(push, 0)
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <expected/tl/expected.hpp>
#pragma warning(pop)


using namespace adsk::core;
using namespace adsk::fusion;


Ptr<Application> app;
Ptr<UserInterface> ui;

const std::string panelId = "SketchPanel";
const std::string myControlId = "RenameDimension";


class Err {
public:
	Err(std::string message) : message(message) {}
	std::string message;
};


bool endswith(std::string s, std::string sub) {
	return (s.find(sub, 0) + sub.length()) == s.length();
}


bool isADimension(std::string objectType) {
	// this is offensive code and I wish I could just check if the object is a subclass of SketchDimension
	return objectType.rfind("adsk::fusion::Sketch", 0) == 0 && endswith(objectType, "Dimension");
}


tl::expected<int, Err> handleRename() {
	auto selections = ui->activeSelections();
	if (!selections) return tl::make_unexpected(Err("activeSelections is null"));

	if (selections->count() != 1) return tl::make_unexpected(Err("Must have exactly one selection"));

	auto sel = selections->item(0);
	if (!sel) return tl::make_unexpected(Err("Your selection is null. WTF?"));
	auto ent = sel->entity();
	if (!ent) return tl::make_unexpected(Err("Your selection references a null thing. WTF?"));

	std::string entType = ent->objectType();
	if (!isADimension(entType)) {
		std::stringstream ss;
		ss << "Selected " << entType << " is not a dimension";
		return tl::make_unexpected(Err(ss.str()));
	}

	auto dim = ent->cast<SketchDimension>();
	if (!dim) return tl::make_unexpected(Err("Given dimension is null"));
	auto param = dim->parameter();
	if (!param) return tl::make_unexpected(Err("Given dimension has a null parameter"));

	bool cancelled;
	auto newName = ui->inputBox("New dimension name?", cancelled, "", param->name());
	if (!cancelled) param->name(newName);
	return 1;
}


class CommandExecutedHandler : public CommandEventHandler {
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override {
		Ptr<Event> firingEvent = eventArgs->firingEvent();
		if (!firingEvent) return;

		Ptr<Command> command = firingEvent->sender();
		if (!command) return;

		Ptr<CommandDefinition> cmdDef = command->parentCommandDefinition();
		if (!cmdDef) return;

		if (cmdDef->id() != myControlId) return;

		auto res = handleRename();
		if (!res.has_value()) ui->messageBox(res.error().message);
		
	}
};

class CommandCreatedHandler : public CommandCreatedEventHandler {
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) {
		if (!eventArgs) return;
		Ptr<Command> cmd = eventArgs->command();
		if (!cmd) return;

		Ptr<CommandEvent> exec = cmd->execute();
		if (!exec) return;

		exec->add(&onCommandExecuted_);
	}
private:
	CommandExecutedHandler onCommandExecuted_;
};

CommandCreatedHandler onCommandCreated;

tl::expected<Ptr<CommandDefinition>, Err> getOrCreateControl(Ptr<ToolbarControls> controls) {
	auto myControl = controls->itemById(myControlId);
	if (myControl) {
		return myControl;
	}

	auto cmdDefs = ui->commandDefinitions();
	if (!cmdDefs) {
		return tl::make_unexpected(Err("Could not retrieve command definitions"));
	}

	auto btn = cmdDefs->itemById(myControlId);
	if (!btn) {
		btn = cmdDefs->addButtonDefinition(myControlId, "Rename Dimension", "");
		if (!btn) {
			return tl::make_unexpected(Err("Could not make button"));
		}
	}

	auto creationEvent = btn->commandCreated();
	if (!creationEvent) return tl::make_unexpected(Err("Failed to get creation event"));
	creationEvent->add(&onCommandCreated);

	auto btnCommand = controls->addCommand(btn);
	if (!btnCommand) {
		return tl::make_unexpected(Err("Failed to add command to toolbar controls"));
	}
	btnCommand->isVisible(true);
	return btnCommand;
}

tl::expected<Ptr<ToolbarControls>, Err> getSketchPanelControls() {
	Ptr<ToolbarPanelList> panels = ui->allToolbarPanels();
	if (!panels) return tl::make_unexpected(Err("Could not retrieve all panels"));

	Ptr<ToolbarPanel> panel = panels->itemById(panelId);
	if (!panel) return tl::make_unexpected(Err("Could not retrieve Sketch panel"));

	Ptr<ToolbarControls> controls = panel->controls();
	if (!controls) return tl::make_unexpected(Err("Could not retrieve Sketch panel controls"));
	return controls;
}

extern "C" XI_EXPORT bool run(const char*)
{
	app = Application::get();
	if (!app) return false;

	ui = app->userInterface();
	if (!ui) return false;

	auto controls = getSketchPanelControls();
	auto myControl = controls.map(getOrCreateControl);
	if (!myControl.has_value()) ui->messageBox(myControl.error().message);

	return true;
}

extern "C" XI_EXPORT bool stop(const char*)
{
	if (ui)
	{
		auto controls = getSketchPanelControls();
		controls.map([](Ptr<ToolbarControls> controls) {
			auto myControl = controls->itemById(myControlId);
			if (!myControl) return;
			myControl->deleteMe();
		});
		auto cmdDefs = ui->commandDefinitions();
		auto myCmd = cmdDefs->itemById(myControlId);
		if (myCmd) myCmd->deleteMe();
		ui = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
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
