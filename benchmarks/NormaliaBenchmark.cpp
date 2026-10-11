#include "NormaliaBenchmark.h"

std::vector<ga_data> documented_normalia_route() {
    std::vector<ga_data> route;
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_buy, 10, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_merchant, m_Magic_Cleppsydra);
    route.emplace_back(_a_buy, 11, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 30, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_witch, w_Midas_was_a_Trader);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 18, 1, 12, 0, 1, 1);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_buy, 27, 0, 1, 0, 0, 5);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_interest);
    route.emplace_back(_a_interest);
    return route;
}

std::vector<ga_data> improved_normalia_route() {
    std::vector<ga_data> route;
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_buy, 10, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_merchant, m_Wooden_Statuette);
    route.emplace_back(_a_buy, 11, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 30, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_witch, w_Midas_was_a_Trader);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 18, 9, 0, 0, 0, 2);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_buy, 18, 0, 0, 0, 0, 7);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_interest);
    return route;
}

std::vector<ga_data> record_normalia_route() {
    std::vector<ga_data> route;
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_buy, 12, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 26, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_merchant, m_Magic_Cleppsydra);
    route.emplace_back(_a_npc, n_Trader);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_witch, w_Midas_was_a_Trader);
    route.emplace_back(_a_buy, 43, 0, 0, 0, 0, 0);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_npc, n_Camel);
    route.emplace_back(_a_buy, 2, 8, 9, 0, 1, 2);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_buy, 20, 0, 0, 0, 0, 9);
    route.emplace_back(_a_sell);
    route.emplace_back(_a_interest);
    return route;
}
