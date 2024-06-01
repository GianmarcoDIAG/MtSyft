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
#include "CommonCoreChainSynthesizer.h"
using namespace std;

int main(int argc, char** argv)
{
    CLI::App app {
        "cb-MtSyft: A tool for LTLf best-effort synthesis in multi-tier environments with a common base"
    };

    std::string goal_file, env_file, part_file;
    int starting_flag;

    CLI::Option* goal_opt =
        app.add_option("-a,--agent-file", goal_file, "File to agent specification")->
        required() -> check(CLI::ExistingFile);
        
    CLI::Option* env_opt =
        app.add_option("-e,--environment-file", env_file, "File to environment tiers E_c, E'_1, ..., E'_n (E_c base env., E'_1 most determined refinemnt, E'_n least determined refinement)")->
        required() -> check(CLI::ExistingFile);

    CLI::Option* part_opt =
        app.add_option("-p,--partition-file", part_file, "File to partition" )->
        required () -> check(CLI::ExistingFile);

    CLI::Option* starting_opt =
        app.add_option("-s,--starting-player", starting_flag, "Starting player (agent=1, environment=0)")->
        required();

    bool benchmark_testing = false;
    app.add_flag("-t,--store-results", benchmark_testing, "Specifies results should be stored in results.csv");

    // TODO: add option to print the strategy(ies)

    CLI11_PARSE(app, argc, argv);

    // agent goal
    string ltlf_goal;
    ifstream goal_spec_stream(goal_file);
    getline(goal_spec_stream, ltlf_goal);
    // std::cout << "Goal: " << ltlf_goal << std::endl;

    // env specifications
    string env_core;
    ifstream env_specs_stream(env_file);
    getline(env_specs_stream, env_core); // first line contains core specification
    // std::cout << "Core: " << env_core << std::endl;

    string env_conjunct;
    vector<string> env_conjuncts;
    // std::cout << "Environment conjuncts" << std::endl;
    while (getline(env_specs_stream, env_conjunct)) {
        // std::cout << env_conjunct << std::endl;
        env_conjuncts.push_back(env_conjunct); // conjuncts E'_i follow line by line core specification
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

    Syft::CommonCoreChainSynthesizer chain_best_effort_synthesizer(
        v_mgr,
        ltlf_goal,
        env_core,
        env_conjuncts,
        partition,
        starting_player
    );

    Syft::ChainedStrategies results = chain_best_effort_synthesizer.run();

    chain_best_effort_synthesizer.realizability(results.adversarial_results, results.cooperative_results);

    double MILLISEC_PER_SEC = 1000.0, running_time = timer.stop().count() / MILLISEC_PER_SEC;
    std::cout << "[cb-MtSfyt] Runnin time " << running_time << " s" << std::endl;

    if (benchmark_testing) {
        // std::ofstream out_stream("cccsyft-results.csv", std::ofstream::app);
        std::ofstream out_stream("res_cb_mtsyft.csv", std::ofstream::app);
        out_stream << goal_file << "," << env_file << "," << running_time << std::endl;

        // std::ofstream out_stream2("cccsyft-times.csv", std::ofstream::app);
        std::ofstream out_stream2("times_cb_mtsyft.csv", std::ofstream::app);
        auto op_times = chain_best_effort_synthesizer.get_run_times();
        out_stream2 << goal_file << "," << env_file << "," << op_times[0] << "," << op_times[1] << "," << op_times[2] << "," << op_times[3] << std::endl;
    }
}