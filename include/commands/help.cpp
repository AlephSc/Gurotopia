#include "pch.hpp"
#include "help.hpp"

void help(ENetEvent& event, const std::string_view text)
{
    ::peer *perper = static_cast<::peer*>(event.peer->data);

    std::string helpText = "`wAvailable commands:`` /warp /sb /who /me /wave /dance /love /sleep /facepalm /fp /smh /yes /no /omg /idk /shrug /furious /rolleyes /foldarms /stubborn /fold /dab /sassy /dance2 /march /grumpy /shy`w.``\n";

    helpText += (perper->role > 0) ? "\n`#Dou Zun commands:`` /punch /skin /ghost /find`w.``" : "";
    helpText += (perper->role > 1) ? "\n`8Ban Sheng commands:`` /weather /nick`w.``" : "";
    helpText += (perper->role > 2) ? "\n`4Dou Sheng commands:`` /ageworld`w.``" : "";
    helpText += (perper->role > 3) ? "\n`cDou Di commands:`` /edit`w.``" : "";

    packet::create(*event.peer, false, 0, { "OnConsoleMessage", helpText.c_str() 
    });
}