
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


Ptr<SketchDimension> getSelectedDimension(Ptr<Selections> selections) {
	if (!selections) return nullptr;

	if (selections->count() != 1) return nullptr;

	auto sel = selections->item(0);
	if (!sel) return nullptr;
	auto ent = sel->entity();
	if (!ent) return nullptr;

	// this will be null if the cast is invalid because of the fancy Ptr type
	return ent->cast<SketchDimension>();
}

tl::expected<int, Err> handleRename() {
	auto selections = ui->activeSelections();

	auto dim = getSelectedDimension(selections);
	if (!dim) return tl::make_unexpected(Err("Given dimension is null"));
	auto param = dim->parameter();
	if (!param) return tl::make_unexpected(Err("Given dimension has a null parameter"));

	bool cancelled;
	auto newName = ui->inputBox("New dimension name?", cancelled, "", param->name());
	if (!cancelled) param->name(newName);
	return 1;
}

class RenameMenuEventHandler : public MarkingMenuEventHandler {
	void notify(const Ptr<MarkingMenuEventArgs>& args) {
		auto menu = args->linearMarkingMenu();
		if (!menu) return;
		auto controls = menu->controls();
		if (!getSelectedDimension(ui->activeSelections())) {
			auto myControl = controls->itemById(myControlId);
			if (!myControl) return;
			myControl->deleteMe();
		}

		auto cmdDefs = ui->commandDefinitions();
		if (!cmdDefs) return;
		auto myCmd = cmdDefs->itemById(myControlId);
		if (!myCmd) return;

		auto myControl = controls->addCommand(myCmd);
	}
};

RenameMenuEventHandler onMenu;


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

tl::expected<Ptr<CommandDefinition>, Err> getOrCreateCommandDef() {
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
	return btn;
}

extern "C" XI_EXPORT bool run(const char*)
{
	app = Application::get();
	if (!app) return false;

	ui = app->userInterface();
	if (!ui) return false;

	auto cmdDef = getOrCreateCommandDef();
	if (!cmdDef.has_value()) {
		ui->messageBox(cmdDef.error().message);
		return false;
	}

	ui->markingMenuDisplaying()->add(&onMenu);

	return true;
}

extern "C" XI_EXPORT bool stop(const char*)
{
	if (ui)
	{
		ui->markingMenuDisplaying()->remove(&onMenu);
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
