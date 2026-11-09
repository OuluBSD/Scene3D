#include "CardBoardEditor.h"

using namespace Upp;

GUI_APP_MAIN
{
	const Vector<String>& args = CommandLine();
	int cli_code = RunCardBoardEditorCli(args);
	if(cli_code >= 0)
		std::_Exit(cli_code);
	CardBoardEditorWindow window;
	window.Run();
}
