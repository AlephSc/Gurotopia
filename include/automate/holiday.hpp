#pragma once

enum holiday : u_char
{
    H_NONE,
    H_VALENTINES
};

extern std::tm localtime();

extern void check_for_holiday();

extern std::string game_theme_string();

extern std::pair<std::string, std::string> holiday_greeting();

extern u_char holiday;
