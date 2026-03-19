#include "pch.hpp"
#include "on/Action.hpp"
#include "find.hpp"
#include "warp.hpp"
#include "edit.hpp"
#include "punch.hpp"
#include "skin.hpp"
#include "sb.hpp"
#include "who.hpp"
#include "me.hpp"
#include "nick.hpp"
#include "help.hpp"
#include "weather.hpp"
#include "ghost.hpp"
#include "ageworld.hpp"
#include "__command.hpp"

/* if you plan to use this outside of this file, please include in __command.hpp (^-^) - and just make it a void. */


std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool
{
    {"help", &help},
    {"?", &help},
    {"find", &find},
    {"warp", &warp},
    {"edit", &edit},
    {"punch", &punch},
    {"skin", &skin},
    {"sb", &sb},
    {"who", &who},
    {"me", &me},
    {"nick", &nick},
    {"weather", &weather},
    {"ghost", &ghost},
    {"ageworld", &ageworld},
    {"wave", &on::Action}, {"dance", &on::Action}, {"love", &on::Action}, {"sleep", &on::Action}, {"facepalm", &on::Action}, {"fp", &on::Action}, 
    {"smh", &on::Action}, {"yes", &on::Action}, {"no", &on::Action}, {"omg", &on::Action}, {"idk", &on::Action}, {"shrug", &on::Action}, 
    {"furious", &on::Action}, {"rolleyes", &on::Action}, {"foldarms", &on::Action}, {"fa", &on::Action}, {"stubborn", &on::Action}, {"fold", &on::Action}, 
    {"dab", &on::Action}, {"sassy", &on::Action}, {"dance2", &on::Action}, {"march", &on::Action}, {"grumpy", &on::Action}, {"shy", &on::Action}
};