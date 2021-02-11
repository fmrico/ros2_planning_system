// Copyright 2019 Intelligent Robotics Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <map>
#include <set>
#include <tuple>

#include "ament_index_cpp/get_package_share_directory.hpp"

#include "gtest/gtest.h"

#include "plansys2_problem_expert/ProblemExpert.hpp"
#include "plansys2_domain_expert/DomainExpert.hpp"
#include "plansys2_domain_expert/DomainExpertNode.hpp"
#include "plansys2_problem_expert/ProblemExpertNode.hpp"
#include "plansys2_problem_expert/ProblemExpertClient.hpp"
#include "plansys2_problem_expert/Utils.hpp"
#include "plansys2_pddl_parser/Tree.h"

#include "plansys2_msgs/msg/knowledge.hpp"

TEST(utils, evaluate_predicate_use_state)
{
  std::set<std::string> predicates;
  std::map<std::string, double> functions;
  auto test_node = rclcpp::Node::make_shared("test_problem_expert_node");
  auto problem_client = std::make_shared<plansys2::ProblemExpertClient>(test_node);

  auto test_tree_node = parser::pddl::tree::get_tree_node(
    "(patrolled wp1)", false, parser::pddl::tree::AND);

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, false, true),
    std::make_tuple(true, false, 0));

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, false, true, true),
    std::make_tuple(true, true, 0));

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, true, true),
    std::make_tuple(true, true, 0));
  ASSERT_EQ(predicates.size(), 1);
  ASSERT_EQ(*predicates.begin(), "(patrolled wp1)");

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, false, true),
    std::make_tuple(true, true, 0));

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, false, true, true),
    std::make_tuple(true, false, 0));

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions, true, true, true),
    std::make_tuple(true, false, 0));
  ASSERT_TRUE(predicates.empty());
}

TEST(utils, evaluate_function_use_state)
{
  std::set<std::string> predicates;
  std::map<std::string, double> functions;
  auto test_node = rclcpp::Node::make_shared("test_problem_expert_node");

  auto test_tree_node = parser::pddl::tree::get_tree_node(
    "(distance wp1 wp2)", false, parser::pddl::tree::EXPRESSION);

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, predicates, functions),
    std::make_tuple(true, false, 0.0));

  functions["(distance wp1 wp2)"] = 1.0;

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, predicates, functions),
    std::make_tuple(true, false, 1.0));
}

TEST(utils, evaluate_number)
{
  std::set<std::string> predicates;
  std::map<std::string, double> functions;
  auto test_node = rclcpp::Node::make_shared("test_problem_expert_node");
  auto problem_client = std::make_shared<plansys2::ProblemExpertClient>(test_node);

  auto test_tree_node = parser::pddl::tree::get_tree_node(
    "3.0", false, parser::pddl::tree::EXPRESSION);

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions),
    std::make_tuple(true, true, 3.0));
}

TEST(utils, evaluate_invalid)
{
  std::set<std::string> predicates;
  std::map<std::string, double> functions;
  auto test_node = rclcpp::Node::make_shared("test_problem_expert_node");
  auto problem_client = std::make_shared<plansys2::ProblemExpertClient>(test_node);

  ASSERT_EQ(
    plansys2::evaluate(NULL, problem_client, predicates, functions),
    std::make_tuple(true, true, 0));

  auto test_tree_node = parser::pddl::tree::get_tree_node(
    "(patrolled wp1)", false, parser::pddl::tree::AND);
  test_tree_node->type_ = parser::pddl::tree::UNKNOWN_NODE_TYPE;

  ASSERT_EQ(
    plansys2::evaluate(test_tree_node, problem_client, predicates, functions),
    std::make_tuple(false, false, 0));
}

TEST(utils, get_subtrees)
{
  std::vector<std::shared_ptr<parser::pddl::tree::TreeNode>> empty_expected;

  ASSERT_EQ(plansys2::get_subtrees(NULL), empty_expected);

  plansys2::Goal invalid_goal;
  invalid_goal.fromString("(or (patrolled wp1) (patrolled wp2))");
  ASSERT_EQ(plansys2::get_subtrees(invalid_goal.root_), empty_expected);

  std::vector<std::shared_ptr<parser::pddl::tree::TreeNode>> expected;
  expected.push_back(
    parser::pddl::tree::get_tree_node(
      "(patrolled wp1)", false,
      parser::pddl::tree::AND));
  expected.push_back(
    parser::pddl::tree::get_tree_node(
      "(patrolled wp2)", false,
      parser::pddl::tree::AND));

  plansys2::Goal goal;
  goal.fromString("(and (patrolled wp1) (patrolled wp2))");
  auto actual = plansys2::get_subtrees(goal.root_);
  ASSERT_EQ(actual.size(), expected.size());
  for (size_t i = 0; i < expected.size(); i++) {
    ASSERT_EQ(actual[i]->toString(), expected[i]->toString());
  }
}

TEST(utils, get_action_from_string)
{
  auto test_node = rclcpp::Node::make_shared("test_node");
  auto domain_node = std::make_shared<plansys2::DomainExpertNode>();
  auto domain_client = std::make_shared<plansys2::DomainExpertClient>(test_node);

  std::string pkgpath = ament_index_cpp::get_package_share_directory("plansys2_problem_expert");

  domain_node->set_parameter({"model_file", pkgpath + "/pddl/domain_simple.pddl"});

  domain_node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);
  domain_node->trigger_transition(lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);

  rclcpp::executors::MultiThreadedExecutor exe(rclcpp::executor::ExecutorArgs(), 8);

  exe.add_node(domain_node->get_node_base_interface());

  bool finish = false;
  std::thread t([&]() {
      while (!finish) {exe.spin_some();}
    });

  std::string action_str = "(teleport r2d2 kitchen bedroom)";

  std::shared_ptr<parser::pddl::tree::DurativeAction> expected =
    std::make_shared<parser::pddl::tree::DurativeAction>();
  expected->name = "teleport";
  expected->parameters.push_back(parser::pddl::tree::Param{"r2d2", ""});
  expected->parameters.push_back(parser::pddl::tree::Param{"kitchen", ""});
  expected->parameters.push_back(parser::pddl::tree::Param{"bedroom", ""});
  expected->at_start_requirements.fromString(
    "(and "
    "(robot_at r2d2 kitchen)(is_teleporter_enabled kitchen)(is_teleporter_destination bedroom))");
  expected->at_end_effects.fromString("(and (not(robot_at r2d2 kitchen))(robot_at r2d2 bedroom))");

  std::shared_ptr<parser::pddl::tree::DurativeAction> actual =
    plansys2::get_action_from_string(action_str, domain_client);

  ASSERT_EQ(actual->name_actions_to_string(), expected->name_actions_to_string());
  ASSERT_EQ(actual->at_start_requirements.toString(), expected->at_start_requirements.toString());
  ASSERT_EQ(actual->over_all_requirements.toString(), expected->over_all_requirements.toString());
  ASSERT_EQ(actual->at_end_requirements.toString(), expected->at_end_requirements.toString());
  ASSERT_EQ(actual->at_start_effects.toString(), expected->at_start_effects.toString());
  ASSERT_EQ(actual->at_end_effects.toString(), expected->at_end_effects.toString());

  std::string durative_action_str = "(move r2d2 kitchen bedroom)";

  std::shared_ptr<parser::pddl::tree::DurativeAction> durative_expected =
    std::make_shared<parser::pddl::tree::DurativeAction>();
  durative_expected->name = "move";
  durative_expected->parameters.push_back(parser::pddl::tree::Param{"r2d2", ""});
  durative_expected->parameters.push_back(parser::pddl::tree::Param{"kitchen", ""});
  durative_expected->parameters.push_back(parser::pddl::tree::Param{"bedroom", ""});
  durative_expected->at_start_requirements.fromString("(and (robot_at r2d2 kitchen))");
  durative_expected->at_start_effects.fromString("(and (not(robot_at r2d2 kitchen)))");
  durative_expected->at_end_effects.fromString("(and (robot_at r2d2 bedroom))");

  std::shared_ptr<parser::pddl::tree::DurativeAction> durative_actual =
    plansys2::get_action_from_string(durative_action_str, domain_client);

  ASSERT_EQ(durative_actual->name_actions_to_string(), durative_expected->name_actions_to_string());
  ASSERT_EQ(
    durative_actual->at_start_requirements.toString(),
    durative_expected->at_start_requirements.toString());
  ASSERT_EQ(
    durative_actual->over_all_requirements.toString(),
    durative_expected->over_all_requirements.toString());
  ASSERT_EQ(
    durative_actual->at_end_requirements.toString(),
    durative_expected->at_end_requirements.toString());
  ASSERT_EQ(
    durative_actual->at_start_effects.toString(), durative_expected->at_start_effects.toString());
  ASSERT_EQ(
    durative_actual->at_end_effects.toString(),
    durative_expected->at_end_effects.toString());

  finish = true;
  t.join();
}

TEST(utils, get_params)
{
  std::string action_str = "(move r2d2 bedroom)";

  std::vector<std::string> expected;
  expected.push_back("r2d2");
  expected.push_back("bedroom");

  ASSERT_EQ(plansys2::get_params(action_str), expected);
}

TEST(utils, get_name)
{
  std::string action_str = "(move r2d2 bedroom)";

  ASSERT_EQ(plansys2::get_name(action_str), "move");
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);

  return RUN_ALL_TESTS();
}
