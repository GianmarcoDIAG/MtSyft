#include <string>
#include <istream>
#include <iostream>
#include <vector>
#include <CLI/CLI.hpp>
#include "Stopwatch.h"
#include "InputOutputPartition.h"
#include "EnvironmentsChainBestEffortSynthesizer.h"
#include "VarMgr.h"
#include "Transducer.h"
using namespace std;

int main(int argc, char **argv)
{

    CLI::App app {
        "MtSyft: A tool for LTLf best-effort synthesis in multi-tier environments"
    };

    std::string goal_file, env_file, part_file;
    int starting_flag;
    
    CLI::Option* goal_opt =
        app.add_option("-a,--agent-file", goal_file, "File to agent specification")->
            required() -> check(CLI::ExistingFile);

    CLI::Option* env_opt =
        app.add_option("-e,--environment-file", env_file, "File to environment tiers E_1, ..., E_n (E_1 most determined, E_n least determined)")->
            required() -> check(CLI::ExistingFile);

    CLI::Option* part_opt =
        app.add_option("-p,--partition-file", part_file, "File to partition" )->
            required () -> check(CLI::ExistingFile);

    CLI::Option* starting_opt =
        app.add_option("-s,--starting-player", starting_flag, "Starting player (agent=1, environment=0)")->
            required();

    bool benchmark_testing = false; 
    app.add_flag("-t,--store-results", benchmark_testing, "Specifies results should be stored in results.csv");

    bool interactive = false;
    app.add_flag("-i,--interactive", interactive, "Executes the synthesized program in interactive mode");

    CLI11_PARSE(app, argc, argv);

    // read LTLf goal from goal_spec
    string ltlf_goal;
    ifstream goal_spec_stream(goal_file);
    getline(goal_spec_stream, ltlf_goal);

    // read LTLf env specs from env_specs
    string env_spec;
    vector<string> ltlf_envs;
    ifstream env_specs_stream(env_file);
    while (getline(env_specs_stream, env_spec)) {
        ltlf_envs.push_back(env_spec);
    }

    // starting player
    Syft::Player starting_player;
    if (starting_flag == 1) {
        starting_player = Syft::Player::Agent;
    } else {
        starting_player = Syft::Player::Environment;
    }

    // read partition
    Syft::InputOutputPartition partition =
        Syft::InputOutputPartition::read_from_file(part_file);

    // construct variable manager
    std::shared_ptr<Syft::VarMgr> v_mgr =
        std::make_shared<Syft::VarMgr>();

    Syft::Stopwatch timer;
    timer.start();

    // construct synthesizer object
    Syft::EnvironmentsChainBestEffortSynthesizer chain_best_effort_synthesizer(
        v_mgr,
        ltlf_goal,
        ltlf_envs,
        partition,
        starting_player);

    Syft::ChainedStrategies results = chain_best_effort_synthesizer.run();

    // auto chained_strategies = chain_best_effort_synthesizer.get_chained_strategies(results.first, results.second);
    // chain_best_effort_synthesizer.dump_chained_strategies(chained_strategies); 
    // chain_best_effort_synthesizer.realizability(results.first, results.second); // shows realizability of synthesis for goal and each env specs.
    chain_best_effort_synthesizer.realizability(results.adversarial_results, results.cooperative_results);
    
    double MILLISEC_PER_SEC = 1000.0, running_time = timer.stop().count() / MILLISEC_PER_SEC;

    std::cout << "[MtSyft] Running time: " << running_time  << " s" << std::endl;

    // debug. Prints the strategies
    // for(int i = 0; i < results.adversarial_results.size(); ++i) {
    //     results.adversarial_results[i].transducer.get()->dump_dot("adversarial_"+std::to_string(i+1)+".dot");
    //     results.cooperative_results[i].transducer.get()->dump_dot("cooperative_"+std::to_string(i+1)+".dot");
    // }

    if (interactive) chain_best_effort_synthesizer.interactive(results.adversarial_results, results.cooperative_results);

    if (benchmark_testing) {
        std::ofstream out_stream("res_mtsyft.csv", std::ofstream::app);
        out_stream << goal_file << "," << env_file << "," << running_time << std::endl;

        std::ofstream out_stream2("times_mtsyft.csv", std::ofstream::app);
        auto op_times = chain_best_effort_synthesizer.get_run_times();
        // out_stream2 << goal_file << "," << env_file << "," <<  op_times[0] << "," << op_times[1] << "," << op_times[2] << "," << op_times[3] << "," << op_times[4] << std::endl;
        out_stream2 << goal_file << "," << env_file << "," <<  op_times[0] << "," << op_times[1] << "," << op_times[2] << "," << op_times[3] << std::endl;
    }

    // chain_best_effort_synthesizer.merge_and_dump_dot(results.first, results.second, "best_effort_strategy.dot"); // output the strategy in .dot
    // chain_best_effort_synthesizer.merge_and_dump_dots(results.first, results.second);

    return 0;
}
