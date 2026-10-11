#include "DeferredTraceRecipe.h"

namespace {
bool same_buy(const buy_list& lhs, const buy_list& rhs) {
    return lhs.food == rhs.food && lhs.spice == rhs.spice && lhs.pottery == rhs.pottery &&
           lhs.marble == rhs.marble && lhs.silk == rhs.silk && lhs.jewelry == rhs.jewelry;
}

bool same_action(const SolverAction& lhs, const SolverAction& rhs) {
    return lhs.action == rhs.action && lhs.option == rhs.option && same_buy(lhs.buy, rhs.buy);
}
}

void DeferredTraceRecipe::append(const SolverAction& action) {
    tail = std::make_shared<const Node>(Node{
        action,
        tail,
        tail == nullptr ? 1U : tail->depth + 1U,
    });
}

std::vector<SolverAction> DeferredTraceRecipe::resolve(
    const buy_list& concrete_buy,
    const SolverAction* final_action
) const {
    std::vector<SolverAction> actions;
    const std::vector<SolverAction> suffix = actions_after_buy();
    actions.reserve(1 + suffix.size() + (final_action == nullptr ? 0 : 1));
    actions.push_back(SolverAction{_a_buy, concrete_buy, 0});
    actions.insert(actions.end(), suffix.begin(), suffix.end());
    if (final_action != nullptr) {
        actions.push_back(*final_action);
    }
    return actions;
}

std::vector<SolverAction> DeferredTraceRecipe::actions_after_buy() const {
    std::vector<SolverAction> actions(size());
    std::shared_ptr<const Node> current = tail;
    for (size_t index = actions.size(); index > 0; --index) {
        actions[index - 1] = current->action;
        current = current->previous;
    }
    return actions;
}

size_t DeferredTraceRecipe::size() const noexcept {
    return tail == nullptr ? 0 : tail->depth;
}

size_t DeferredTraceRecipe::memory_bytes() const noexcept {
    std::unordered_set<const void*> counted;
    return sizeof(*this) + dynamic_memory_bytes(counted);
}

size_t DeferredTraceRecipe::dynamic_memory_bytes(
    std::unordered_set<const void*>& counted_shared
) const noexcept {
    size_t bytes = 0;
    std::shared_ptr<const Node> current = tail;
    while (current != nullptr) {
        if (!counted_shared.insert(current.get()).second) {
            break;
        }
        bytes += sizeof(Node) + 2U * sizeof(void*);
        current = current->previous;
    }
    return bytes;
}

bool DeferredTraceRecipe::operator==(const DeferredTraceRecipe& other) const noexcept {
    if (size() != other.size()) {
        return false;
    }
    std::shared_ptr<const Node> lhs = tail;
    std::shared_ptr<const Node> rhs = other.tail;
    while (lhs != nullptr && rhs != nullptr) {
        if (!same_action(lhs->action, rhs->action)) {
            return false;
        }
        lhs = lhs->previous;
        rhs = rhs->previous;
    }
    return true;
}
