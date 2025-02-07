//
// Created by Jay on 1/21/2025.
//

#include "BestPurchaseHandler.h"

BestPurchaseHandler::BestPurchaseHandler() {
    std::filesystem::path savedTreesPath = std::filesystem::current_path().parent_path() / "Saved_Trees";
    for (const auto& entry : std::filesystem::directory_iterator(savedTreesPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".bin")  {
                // std::cout << filename << std::endl;
                prebuiltKeys[filename] = true;
            }
        }
    }
}

std::pair<uint32_t, buy_list> BestPurchaseHandler::getBestBuy(uint16_t cost, uint16_t weight, const Player& player) {
    std::string key = convertWorldToKey(player.player_world);

    if (loadedTrees.find(key) == loadedTrees.end()) {
        if (prebuiltKeys.find(key) != prebuiltKeys.end()) {
            loadTree(key);
        } else {
            buildTree(player, key);
            saveTree(key);
        }
    }

    // Query the tree
    return loadedTrees[key].query(cost, weight);
}

void BestPurchaseHandler::loadTree(const std::string& key) {
    loadedTrees[key] = FenwickTree2D();
    try {
        loadedTrees[key].deserialize(key);
    } catch (const std::exception& e) {
        throw std::runtime_error("Deserialization failed for tree: " + key + ", Error: " + e.what());
    }
}

void BestPurchaseHandler::saveTree(const std::string& key) {
    loadedTrees[key].serialize(key);
}

void BestPurchaseHandler::buildTree(const Player& player, const std::string& key) {
    std::cout << "[*] Building Tree " << key << std::endl;
    std::vector<buy_option> elements;
    generateElementsForWorld(player, elements);
    loadedTrees[key] = FenwickTree2D(elements);
    std::cout << "[*] Build Finished" << std::endl;
}
