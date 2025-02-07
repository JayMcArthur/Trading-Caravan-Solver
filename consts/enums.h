//
// Created by Jay on 6/13/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_ENUMS_H
#define TRADING_CARAVAN_SOLVER_ENUMS_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include <fstream>

enum items : uint8_t {
    i_Food = 0,
    i_Spice,
    i_Pottery,
    i_Marble,
    i_Silk,
    i_Jewelry
};

enum item_data: uint8_t {
    id_Buy = 0,
    id_Sell,
    id_Weight
};

enum npcs: uint8_t {
    n_Trader = 0,
    n_Camel
};

enum npc_data: uint8_t {
    nd_Cost = 0,
    nd_Effect,
    nd_Eat
};

enum merch_options: uint8_t {
    m_No_Merchant = 0,  // Didn't buy or hasn't happened
    m_Cornucopia,       // Food consumption -2
    m_Route_to_Mahaji,  // Spice sell +20
    m_Wooden_Statuette, // Gain a Statue (+500 points per Trader and Camel)
    m_Canvas_Bag,       // Carry Weight +20
    m_Leaders_Necklace, // Gain one Trader
    m_Hand_of_Midas,    // When you sell everything -> Gold +100
    m_Sturdy_Saddle,    // Camels weight +20
    m_Magic_Cleppsydra, // Max Day +1
    m_Blue_Treasure     // Interest Rate +20%
};
const merch_options ALL_MERCH_OPTIONS[] = {
        m_Cornucopia,
        m_Route_to_Mahaji,
        m_Wooden_Statuette,
        m_Canvas_Bag,
        m_Leaders_Necklace,
        m_Hand_of_Midas,
        m_Sturdy_Saddle,
        m_Magic_Cleppsydra,
        m_Blue_Treasure
    };
const std::unordered_map<merch_options, const std::string > mo_conversion = {
        {m_No_Merchant,      "No Merchant"},
        {m_Cornucopia,       "Cornucopia"},
        {m_Route_to_Mahaji,  "Route to Mahaji"},
        {m_Wooden_Statuette, "Wooden Statuette"},
        {m_Canvas_Bag,       "Canvas Bag"},
        {m_Leaders_Necklace, "Leaders Necklace"},
        {m_Hand_of_Midas,    "Hand of Midas"},
        {m_Sturdy_Saddle,    "Sturdy Saddle"},
        {m_Magic_Cleppsydra, "Magic Cleppsydra"},
        {m_Blue_Treasure,    "Blue Treasure"},
};

enum witch_options: uint8_t {
    w_No_Witch = 0,       // Witch hasn't happened yet
    w_Presents_Gift,      // Carry Weight +20        -- Interest Rate -20%
    w_Vertue_of_Patience, // Interest Rate is stable -- Jewelry cost +100
    w_Midas_was_a_Trader, // Trader income +150      -- Marble sell *0
    w_Camelization,       // Camel weight +10        -- Camel eat 2x
    w_Time_is_Money,      // Max Day +1              -- Jewelry sell -500
    w_Animal_Lover,       // Gain two Camels         -- Max Day -1
    w_Oasis_of_Sanctifan, // Crew don't eat          -- Silk buy +200
    w_The_Stonecutter     // Marble weight -15       -- Marble cost +60
};
const witch_options ALL_WITCH_OPTIONS[] = {
        w_Presents_Gift,
        w_Vertue_of_Patience,
        w_Midas_was_a_Trader,
        w_Camelization,
        w_Time_is_Money,
        w_Animal_Lover,
        w_Oasis_of_Sanctifan,
        w_The_Stonecutter
};
const std::unordered_map<witch_options, const std::string> wo_conversion = {
        {w_No_Witch,           "No Witch"},
        {w_Presents_Gift,      "Present's Gift"},
        {w_Vertue_of_Patience, "Vertue of Patience"},
        {w_Midas_was_a_Trader, "Midas was a Trader"},
        {w_Camelization,       "Camelization"},
        {w_Time_is_Money,      "Time is Money"},
        {w_Animal_Lover,       "Animal Lover"},
        {w_Oasis_of_Sanctifan, "Oasis of Sanctifan"},
        {w_The_Stonecutter, "The Stonecutter"}
};

enum town_options: uint8_t {
    t_Normalia = 0, // 00 - No Effect
    t_Fang_Mo,      // 01 - Begin with 1 Pottery
    t_L_Exquise,    // 02 - Traders don't need Food
    t_Mahaji,       // 03 - Spice sell +5
    t_Tocaccialli,  // 04 - Max Day +1
    t_Bajaar,       // 05 - x2 Trader income
    t_Doba_Lao,     // 06 - Interest Rate +20%
    t_Huangoz,      // 07 - Marble Cost -20
    t_Port_Willam,  // 08 - Carry Weight +15
    t_Moonin,       // 09 - Call Merchant at anytime
    t_Cornecopea,   // 10 - Food is free
    t_Eduming,      // 11 - Buying trader -> Interest Rate +10%
    t_Octoyashi,    // 12 - Begin with +3 Food
    t_Kifuai,       // 13 - Begin with +10 Gold
    t_Skjollird,    // 14 - Camel cost +10 and weight +10
    t_Petrinov,     // 15 - Interest rate doesn't decrease
    t_Jilliqo,      // 16 - Daily Gold Income +10
    t_Gayawaku,     // 17 - Food is weightless
    t_Vilparino,    // 18 - Strange Merchant 20% Sale!
    t_Sanctifan,    // 19 - One Free Collect Interest Action
    t_Mehaz,        // 20 - Camels don't need food
    t_Ipartus       // 21 - Silk Cost -100
};
const std::unordered_map<town_options, const std::string > to_conversion = {
        {t_Normalia,    "Normalia"},
        {t_Fang_Mo,     "Fang-Mo"},
        {t_L_Exquise,   "L'Exquise"},
        {t_Mahaji,      "Mahaji"},
        {t_Tocaccialli, "Tocaccialli"},
        {t_Bajaar,      "Bajaar"},
        {t_Doba_Lao,    "Doba Lao"},
        {t_Huangoz,     "Huangoz"},
        {t_Port_Willam, "Port-Willam"},
        {t_Moonin,     "Moonin"},
        {t_Cornecopea, "Cornecopea"},
        {t_Eduming,   "Eduming"},
        {t_Octoyashi, "Octoyashi"},
        {t_Kifuai,    "Kifuai"},
        {t_Skjollird, "Skjollird"},
        {t_Petrinov,  "Petrinov"},
        {t_Jilliqo,   "Jilliqo"},
        {t_Gayawaku,  "Gayawaku"},
        {t_Vilparino, "Vilparino"},
        {t_Sanctifan, "Sanctifan"},
        {t_Mehaz,     "Mehaz"},
        {t_Ipartus,   "Ipartus"},
};

struct world {
    uint8_t town;
    uint8_t merch;
    uint8_t witch;

    world() {
        town = t_Normalia;
        merch = m_No_Merchant;
        witch = w_No_Witch;
    };
};

struct buy_list {
    uint8_t food;
    uint8_t spice;
    uint8_t pottery;
    uint8_t marble;
    uint8_t silk;
    uint8_t jewelry;

    buy_list() : food(0), spice(0), pottery(0), marble(0), silk(0), jewelry(0) {};

    buy_list(const uint8_t &_food, const uint8_t &_spice, const uint8_t &_pottery, const uint8_t &_marble, const uint8_t &_silk, const uint8_t &_jewelry) {
        food = _food;
        spice = _spice;
        pottery = _pottery;
        marble = _marble;
        silk = _silk;
        jewelry = _jewelry;
    };

    friend std::ostream &operator<<(std::ostream &os, const buy_list &obj) {
        os << "buy_list(food: " << static_cast<int>(obj.food)
           << ", spice: " << static_cast<int>(obj.spice)
           << ", pottery: " << static_cast<int>(obj.pottery)
           << ", marble: " << static_cast<int>(obj.marble)
           << ", silk: " << static_cast<int>(obj.silk)
           << ", jewelry: " << static_cast<int>(obj.jewelry) << ")";
        return os;
    };

    void serialize(std::ofstream &out) const {
        out.write(reinterpret_cast<const char*>(&food), sizeof(food));
        out.write(reinterpret_cast<const char*>(&spice), sizeof(spice));
        out.write(reinterpret_cast<const char*>(&pottery), sizeof(pottery));
        out.write(reinterpret_cast<const char*>(&marble), sizeof(marble));
        out.write(reinterpret_cast<const char*>(&silk), sizeof(silk));
        out.write(reinterpret_cast<const char*>(&jewelry), sizeof(jewelry));
    }

    // Deserialize
    void deserialize(std::ifstream &in) {
        in.read(reinterpret_cast<char*>(&food), sizeof(food));
        in.read(reinterpret_cast<char*>(&spice), sizeof(spice));
        in.read(reinterpret_cast<char*>(&pottery), sizeof(pottery));
        in.read(reinterpret_cast<char*>(&marble), sizeof(marble));
        in.read(reinterpret_cast<char*>(&silk), sizeof(silk));
        in.read(reinterpret_cast<char*>(&jewelry), sizeof(jewelry));
    }
};

struct buy_option {
    uint16_t cost;
    uint16_t weight;
    uint32_t profit;
    buy_list buy_data;

    buy_option(const uint16_t& _cost, const uint16_t& _weight, const uint32_t& _profit, const buy_list& _buy_data) {
        cost = _cost;
        weight = _weight;
        profit = _profit;
        buy_data = _buy_data;
    }
};

enum game_actions: uint8_t {
    _a_buy = 0,
    _a_sell,
    _a_npc,
    _a_interest,
    _a_merchant,
    _a_witch
};

struct ga_data {
    game_actions action;
    buy_list buy_data = buy_list(0, 0, 0, 0, 0, 0);
    npcs npc_data {};
    merch_options merch_data {};
    witch_options witch_data {};

    ga_data(game_actions _action, const uint8_t food, const uint8_t spice, const uint8_t pottery, const uint8_t marble, const uint8_t silk, const uint8_t jewelry) {
        action = _action;
        buy_data = buy_list(food, spice, pottery, marble, silk, jewelry);
    };
    ga_data(game_actions _action,  const npcs _npc_data) {
        action = _action;
        npc_data = _npc_data;
    };
    ga_data(game_actions _action, const merch_options _merch_data) {
        action = _action;
        merch_data = _merch_data;
    };
    ga_data(game_actions _action, const witch_options _witch_data) {
        action = _action;
        witch_data = _witch_data;
    };
    explicit ga_data(game_actions _action) {
        action = _action;
    };
};


#endif //TRADING_CARAVAN_SOLVER_ENUMS_H