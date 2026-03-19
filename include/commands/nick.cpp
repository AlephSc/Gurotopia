#include "pch.hpp"
#include "nick.hpp"

void nick(ENetEvent& event, const std::string_view text)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    if (peer->role < DOU_ZUN) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "`4You don't have permission to use this command." });
        return;
    }
    if (text.length() <= sizeof("nick ") - 1) 
    {
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Usage: /nick `w{message}``" });

        packet::create(*event.peer, true, 0, {
            "OnNameChanged",
            std::format("`{}{}``", peer->prefix, peer->ltoken[0]).c_str()
        });

        return;
    }
    std::string nickname{ text.substr(sizeof("nick ")-1) };

    packet::create(*event.peer, false, 0, { "OnConsoleMessage", "Name changed to : `" + nickname });
    packet::create(*event.peer, true, 0, {
        "OnNameChanged",
        std::format("{}``", nickname).c_str()
    });
}