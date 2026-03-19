#pragma once
#include "pch.hpp"

namespace itemConfig {

    struct RuleSet {
        std::unordered_set<type> allowed;
        std::unordered_set<type> denied;
    };

    // =========================
    // RULE BERDASARKAN ROLE (int)
    // =========================
    inline const std::unordered_map<int, RuleSet> rules {

        // role 1 → DOU_ZUN (whitelist)
        {
            1,
            {
                { type::FOREGROUND, type::BACKGROUND },
                {}
            }
        },

        // role 2 → BAN_SHENG (blacklist)
        {
            2,
            {
                {},
                {
                    type::LOCK,
                    type::FIST,
                    type::WRENCH,
                    type::CONSUMEABLE,
                    type::GEM,
                    type::TREASURE,
                    type::MAIN_DOOR,
                    type::STRONG,
                    type::SEED,
                    type::CLOTHING,
                    type::PINATA,
                    type::PROVIDER,
                    type::LOCK_BOT
                }
            }
        },

        // role 3 → DOU_SHENG
        {
            3,
            {
                {},
                {
                    type::LOCK,
                    type::FIST,
                    type::WRENCH,
                    type::GEM,
                    type::SEED
                }
            }
        },

        // role 4 → DOU_DI (bebas semua)
        {
            4,
            {
                {},
                {}
            }
        }
    };

    // =========================
    // 🔥 FUNGSI UTAMA
    // =========================
    inline bool isAllowed(int role, type t) {

        auto it = rules.find(role);

        // ⚠️ role tidak ada → TOLAK (lebih aman)
        if (it == rules.end()) {
            return false;
        }

        const auto& rule = it->second;

        // whitelist prioritas
        if (!rule.allowed.empty()) {
            return rule.allowed.contains(t);
        }

        return !rule.denied.contains(t);
    }

}