import TCSC
import random

class AIAgent:
    def __init__(self):
        self.solver = TCSC.AISolver()
        self.state = None

    def get_action(self):
        """
        Use the state to decide the next action.
        For now, we'll use random actions (replace this with your ML model later).
        """
        possible_actions = self.state.get("available_actions", [])
        if not possible_actions:
            return None  # No valid actions
        return random.choice(possible_actions)  # Random action for now

    def run_episode(self):
        """
        Runs a full episode by interacting with the C++ environment.
        """
        self.solver.reset()
        more_to_process = True
        count = 0

        while more_to_process:
            self.state = self.solver.get_current_state()
            print(f"Current State: {self.state}")

            action = self.get_action()
            if action is None:
                print("This should never happen.")
                continue

            print(f"Taking Action: {action}")
            more_to_process = self.solver.step(action)
            count += 1
            if count % 1000000 == 0:
                print(count)

        total_reward = self.solver.calculate_final_score()
        print(f"Final Reward: {total_reward}")
        return total_reward


if __name__ == "__main__":
    agent = AIAgent()
    agent.run_episode()
