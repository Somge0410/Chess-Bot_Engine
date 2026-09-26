#include "tuner.h"
#include "config.h"
#include "threadpool.h"
#include "TuneEval.h"
#include "base.h"
#include "evaluation.h"
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace Tuner;

#ifndef TAPERED
static_assert(false, "Tuner requires TAPERED to be defined")
#endif

struct WdlMarker
{
    string marker;
    tune_t wdl;
};

struct CoefficientEntry
{
    int16_t value;
    int16_t index;
};

struct Entry
{
    vector<CoefficientEntry> coefficients;
    tune_t wdl;
    bool white_to_move;
    tune_t additional_score;
#if TAPERED
    int32_t phase;
    tune_t endgame_scale;
#endif
};

static const array<WdlMarker, 7> markers
{
    WdlMarker{"1.0", 1},
    WdlMarker{"1-0", 1},
    WdlMarker{"1/2-1/2", 0.5},
    WdlMarker{"0-1", 0},
    WdlMarker{"|White",1},
	WdlMarker{"|Black",0},
	WdlMarker{"|Draw",0.5}
};

static tune_t get_fen_wdl(const string& original_fen, const bool original_white_to_move, const bool white_to_move, const bool side_to_move_wdl)
{
    tune_t wdl;
    bool marker_found = false;
    for (auto& marker : markers)
    {
        if (original_fen.find(marker.marker) != std::string::npos)
        {
            if (marker_found)
            {
                cout << "WDL marker already found on line " << original_fen << endl;
                throw std::runtime_error("WDL marker already found");
            }
            marker_found = true;
            wdl = marker.wdl;
        }
    }

    if(!marker_found)
    {
        stringstream ss(original_fen);
        while (!ss.eof())
        {
            string word;
            ss >> word;
            if (word.starts_with("0."))
            {
                wdl = stod(word);
                marker_found = true;
            }
            else if (word.starts_with("[0."))
            {
                wdl = stod(word.substr(1, word.size() - 2));
                marker_found = true;
            }
        }
    }

    if (!marker_found)
    {
        cout << "WDL marker not found on line " << original_fen << endl;
        throw std::runtime_error("WDL marker not found");
    }

    if(!original_white_to_move && side_to_move_wdl)
    {
        wdl = 1 - wdl;
    }

    return wdl;
}   

static bool get_fen_color_to_move(const string& fen)
{
    string board;
    string side_to_move;
    stringstream ss(fen);
    return (ss >> board >> side_to_move) && side_to_move == "w";
}

static void print_elapsed(high_resolution_clock::time_point start)
{
    const auto now = high_resolution_clock::now();
    const auto elapsed = now - start;
    const auto elapsed_seconds = duration_cast<seconds>(elapsed).count();
    cout << "[" << elapsed_seconds << "s] ";
}

static void get_coefficient_entries(const coefficients_t& coefficients, vector<CoefficientEntry>& coefficient_entries, int32_t parameter_count)
{
    if(coefficients.size() != parameter_count)
    {
        throw runtime_error("Parameter count mismatch");
    }

    for (size_t i = 0; i < coefficients.size(); i++)
    {
        if (coefficients[i] == 0)
        {
            continue;
        }

        const auto coefficient_entry = CoefficientEntry{coefficients[i], static_cast<int16_t>(i)};
        coefficient_entries.push_back(coefficient_entry);
    }
}

static tune_t linear_eval(const Entry& entry, const parameters_t& parameters)
{
    tune_t score = entry.additional_score;
#if TAPERED 
    tune_t midgame = 0;
    tune_t endgame = 0;
    for (const auto& coefficient : entry.coefficients)
    {
        midgame += coefficient.value * parameters[coefficient.index][static_cast<int32_t>(PhaseStages::Midgame)];
        endgame += coefficient.value * parameters[coefficient.index][static_cast<int32_t>(PhaseStages::Endgame)] * entry.endgame_scale;
    }
    score += (midgame * entry.phase + endgame * (24 - entry.phase)) / 24;
#else
    for (const auto& coefficient : entry.coefficients)
    {
        score += coefficient.value * parameters[coefficient.index];
    }
#endif

    return score;
}

static void print_statistics(const parameters_t& parameters, const vector<Entry>& entries)
{
    array<size_t, 2> wins{};
    array<size_t, 2> draws{};
    array<size_t, 2> losses{};
    array<size_t, 2> total{};
    array<tune_t, 2> wdls{};

    size_t min_parameters = std::numeric_limits<uint64_t>::max();
    size_t max_parameters = 0;
    size_t total_parameters = 0;

    for(auto& entry : entries)
    {
        if(entry.wdl == 1)
            wins[entry.white_to_move]++;
        else if(entry.wdl == 0.5)
            draws[entry.white_to_move]++;
        else if (entry.wdl == 0.0)
            losses[entry.white_to_move]++;
        
        total[entry.white_to_move]++;
        wdls[entry.white_to_move] += entry.wdl;

        if(entry.coefficients.size() < min_parameters)
            min_parameters = entry.coefficients.size();
        if (entry.coefficients.size() > max_parameters)
            max_parameters = entry.coefficients.size();

        total_parameters += entry.coefficients.size();
    }

    cout << "Dataset statistics:" << endl;
    cout << "Total positions: " << entries.size() << endl;
    for(int color = 1; color >= 0; color--)
    {
        const auto color_name = color ? "White" : "Black";
        cout << color_name << ": " << total[color] << " (" << (total[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 1.0: " << wins[color] << " (" << (wins[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 0.5: " << draws[color] << " (" << (draws[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " 0.0: " << losses[color] << " (" << (losses[color] * 100.0 / entries.size()) << "%)" << endl;
        cout << color_name << " avg: " << (total[color] ? wdls[color] / total[color] : 0) << endl;
    }

    auto avg_parameters = static_cast<tune_t>(total_parameters) / entries.size();
    cout << "Parameters total: " << parameters.size() << endl;
    cout << "Parameters min: " << min_parameters << endl;
    cout << "Parameters max: " << max_parameters << endl;
    cout << "Parameters avg: " << avg_parameters << endl;
    cout << endl;
}


static string get_parameter_name(int index)
{
    return "PARAM_" + to_string(index);
}

static filesystem::path find_weights_file()
{
    auto directory = filesystem::current_path();
    while (true)
    {
        const auto candidate = directory / "src" / "evaluation" / "weights.cpp";
        if (filesystem::exists(candidate))
        {
            return candidate;
        }

        const auto parent = directory.parent_path();
        if (parent == directory)
        {
            throw runtime_error("Unable to locate src/evaluation/weights.cpp from the current directory");
        }
        directory = parent;
    }
}

static void print_parameter_non_zero_coverage(const parameters_t& parameters, const vector<Entry>& entries)
{
    vector<size_t> non_zero_counts(parameters.size(), 0);

    for (const auto& entry : entries)
    {
        for (const auto& coefficient : entry.coefficients)
        {
            if (coefficient.index < 0)
            {
                continue;
            }

            const auto parameter_index = static_cast<size_t>(coefficient.index);
            if (parameter_index < non_zero_counts.size())
            {
                non_zero_counts[parameter_index]++;
            }
        }
    }

    cout << "Parameter non-zero coverage:" << endl;
    cout << "Total positions: " << entries.size() << endl;

    size_t ever_non_zero = 0;
    for (size_t i = 0; i < non_zero_counts.size(); i++)
    {
        const auto count = non_zero_counts[i];
        if (count > 0)
        {
            ever_non_zero++;
        }

        const tune_t percentage = entries.empty()
            ? static_cast<tune_t>(0)
            : static_cast<tune_t>(100.0) * static_cast<tune_t>(count) / static_cast<tune_t>(entries.size());

        const auto idx = static_cast<int>(i);
        cout << "Param[" << i+PARAM_START << "] " << get_parameter_name(idx+PARAM_START) << ": "
            << count << " / " << entries.size()
            << " (" << percentage << "%)" << endl;
    }

    const tune_t used_percentage = parameters.empty()
        ? static_cast<tune_t>(0)
        : static_cast<tune_t>(100.0) * static_cast<tune_t>(ever_non_zero) / static_cast<tune_t>(parameters.size());

    cout << "Parameters ever non-zero: " << ever_non_zero << " / " << parameters.size()
        << " (" << used_percentage << "%)" << endl;
    cout << endl;
}

static void parse_fen(const bool side_to_move_wdl, const parameters_t& parameters, vector<Entry>& entries, const string& original_fen)
{
    if constexpr (print_data_entries)
    {
        cout << original_fen;
    }

    // Direkt FEN verwenden (keine Quiescence nötig bei ruhigen Positionen)
    EvalResult eval_result = TuneEval::get_fen_eval_result(original_fen);

    Entry entry;
    entry.white_to_move = get_fen_color_to_move(original_fen);
#if TAPERED
    entry.endgame_scale = eval_result.endgame_scale;
#endif
    entry.wdl = get_fen_wdl(original_fen, entry.white_to_move, entry.white_to_move, side_to_move_wdl);
    get_coefficient_entries(eval_result.coefficients, entry.coefficients, static_cast<int32_t>(parameters.size()));
#if TAPERED
    entry.phase = eval_result.game_phase;
#endif
    entry.additional_score = 0;
    if constexpr (TuneEval::includes_additional_score)
    {
        const tune_t score = linear_eval(entry, parameters);
        if constexpr (print_data_entries)
        {
            cout << " Eval: " << score << endl;
        }
        entry.additional_score = eval_result.additional_score;

    }

    entries.push_back(entry);
}

static void read_fens(const DataSource& source, const high_resolution_clock::time_point start, vector<string>& fens)
{
    cout << "Reading " << source.path;
    if (source.position_limit > 0)
    {
        cout << " (" << source.position_limit << " positions)";
    }
    cout << "..." << endl;

    ifstream file(source.path);
    if (!file)
    {
        cout << "Failed to open " << source.path << endl;
        throw runtime_error("Failed to open data source");
    }

    string original_fen;
    while (getline(file, original_fen))
    {
        if (source.position_limit > 0 && fens.size() >= source.position_limit)
        {
            break;
        }

        if (original_fen.empty())
        {
            continue;
        }

        fens.push_back(original_fen);
    }

    print_elapsed(start);
    std::cout << "Read " << fens.size() << " positions from " << source.path << endl;
}

static void parse_fens(ThreadPool& thread_pool, const DataSource& source, const vector<string>& fens, const parameters_t& parameters, const high_resolution_clock::time_point time_start, vector<Entry>& entries)
{
    cout << "Parsing " << fens.size() << " positions..." << endl;
    array<vector<Entry>, data_load_thread_count> thread_entries;
    const auto side_to_move_wdl = source.side_to_move_wdl;
    constexpr int batch_size = 10000;
    mutex mut;
    queue<vector<string>> batches;
    vector<string> current_batch;
    for(const auto& fen : fens)
    {
        current_batch.push_back(fen);
        if (current_batch.size() == batch_size)
        {
            batches.emplace(current_batch);
            current_batch.clear();
        }
    }
    if(!current_batch.empty())
    {
        batches.emplace(current_batch);
    }

    for (int thread_id = 0; thread_id < data_load_thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_entries, &mut, side_to_move_wdl, parameters, &batches, time_start]()
        {
            vector<Entry> entries;

            int position_count = 0;
            while(true)
            {
                vector<string> thread_batch;
                {
                    lock_guard lock(mut);
                    if(batches.empty())
                    {
                        break;
                    }
                    thread_batch = batches.front();
                    batches.pop();
                }

                constexpr auto thread_data_load_print_interval = TuneEval::data_load_print_interval / data_load_thread_count;
                for(auto& fen : thread_batch)
                {
                    parse_fen(side_to_move_wdl, parameters, entries, fen);
                    position_count++;
                    if (thread_id == 0 && position_count % thread_data_load_print_interval == 0)
                    {
                        print_elapsed(time_start);
                        std::cout << "Parsed ~" << position_count * data_load_thread_count << " positions..." << endl;
                    }
                }
            }

            thread_entries[thread_id] = entries;
        });
    }

    thread_pool.wait_for_completion();

    for (int thread_id = 0; thread_id < data_load_thread_count; thread_id++)
    {
        for(const Entry& entry : thread_entries[thread_id])
        {
            entries.push_back(entry);
        }
    }
}

static void load_fens(ThreadPool& thread_pool, const DataSource& source, const parameters_t& parameters, const high_resolution_clock::time_point start, vector<Entry>& entries)
{
    vector<string> fens;
    read_fens(source, start, fens);
    parse_fens(thread_pool, source, fens, parameters, start, entries);
}

static tune_t sigmoid(const tune_t K, const tune_t eval)
{
    return static_cast<tune_t>(1) / (static_cast<tune_t>(1) + exp(-K * eval / static_cast<tune_t>(400)));
}

static tune_t get_average_error(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters, tune_t K)
{
    array<tune_t, thread_count> thread_errors;
    for(int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_errors, &entries, &parameters, K]()
        {
            const auto start = entries.size() * thread_id / thread_count;
            const auto end = entries.size() * (thread_id + 1) / thread_count;
            tune_t error = 0;
            for (int i = start; i < end; i++)
            {
                const auto& entry = entries[i];
                const auto eval = linear_eval(entry, parameters);
                const auto sig = sigmoid(K, eval);
                const auto diff = entry.wdl - sig;
                const auto entry_error = pow(diff, 2);
                error += entry_error;
            }
            thread_errors[thread_id] = error;
        });
    }

    thread_pool.wait_for_completion();

    tune_t total_error = 0;
    for (int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        total_error += thread_errors[thread_id];
    }

    const tune_t avg_error = total_error / static_cast<tune_t>(entries.size());
    return avg_error;
}

static tune_t find_optimal_k(ThreadPool& thread_pool, const vector<Entry>& entries, const parameters_t& parameters)
{
    constexpr tune_t rate = 10;
    constexpr tune_t delta = 1e-5;
    constexpr tune_t deviation_goal = 1e-6;
    tune_t K = 2.5;
    tune_t deviation = 1;

    while (fabs(deviation) > deviation_goal)
    {
        const tune_t up = get_average_error(thread_pool, entries, parameters, K + delta);
        const tune_t down = get_average_error(thread_pool, entries, parameters, K - delta);
        deviation = (up - down) / (2 * delta);
        cout << "Current K: " << K << ", up: " << up << ", down: " << down << ", deviation: " << deviation << endl;
        K -= deviation * rate;
    }

    return K;
}

static void update_single_gradient(parameters_t& gradient, const Entry& entry, const parameters_t& params, tune_t K) {
    const tune_t eval = linear_eval(entry, params);
    const tune_t sig = sigmoid(K, eval);
    const tune_t res = (entry.wdl - sig) * sig * (1 - sig);

#if TAPERED
    const auto mg_base = res * (entry.phase / static_cast<tune_t>(24));
    const auto eg_base = res - mg_base;
#endif

    for (const auto& coefficient : entry.coefficients)
    {
#if TAPERED
        gradient[coefficient.index][static_cast<int32_t>(PhaseStages::Midgame)] += mg_base * coefficient.value;
        gradient[coefficient.index][static_cast<int32_t>(PhaseStages::Endgame)] += eg_base * coefficient.value * entry.endgame_scale;
#else
        gradient[coefficient.index] += res * coefficient.value;
#endif
    }
}

static void compute_gradient(ThreadPool& thread_pool, parameters_t& gradient, const vector<Entry>& entries, const parameters_t& params, tune_t K)
{
    array<parameters_t, thread_count> thread_gradients;
    for(int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        thread_pool.enqueue([thread_id, &thread_gradients, &entries, &params, K]()
        {
            const auto start = entries.size() * thread_id / thread_count;
            const auto end = entries.size() * (thread_id + 1) / thread_count;
#if TAPERED
            parameters_t gradient = parameters_t(params.size(), pair_t{});
#else
            parameters_t gradient = parameters_t(params.size(), 0);
#endif
            for (int i = start; i < end; i++)
            {
                const auto& entry = entries[i];
                update_single_gradient(gradient, entry, params, K);
            }
            thread_gradients[thread_id] = gradient;
        });
    }

    thread_pool.wait_for_completion();

    for (int thread_id = 0; thread_id < thread_count; thread_id++)
    {
        for(auto parameter_index = 0; parameter_index < params.size(); parameter_index++)
        {
#if TAPERED
            gradient[parameter_index][static_cast<int32_t>(PhaseStages::Midgame)] += thread_gradients[thread_id][parameter_index][static_cast<int32_t>(PhaseStages::Midgame)];
            gradient[parameter_index][static_cast<int32_t>(PhaseStages::Endgame)] += thread_gradients[thread_id][parameter_index][static_cast<int32_t>(PhaseStages::Endgame)];
#else
            gradient[parameter_index] += thread_gradients[thread_id][parameter_index];
#endif
        }
    }
}


void Tuner::run(const std::vector<DataSource>& sources)
{
    cout << "Starting tuning" << endl << endl;
    const auto start = high_resolution_clock::now();

    cout << "Starting thread pool..." << endl;
    ThreadPool thread_pool;
    thread_pool.start(thread_count);

    cout << "Getting initial parameters..." << endl;
    auto parameters = TuneEval::get_initial_parameters();
    cout << "Got " << parameters.size() << " parameters" << endl;

    cout << "Initial parameters:" << endl;
    TuneEval::print_parameters(parameters);

    vector<Entry> entries;

    vector<string> fens;
    for (const auto& source : sources)
    {
        load_fens(thread_pool, source, parameters, start, entries);
    }
    cout << "Data loading complete" << endl << endl;

    if (entries.empty())
    {
        throw runtime_error("No positions were loaded from the data sources");
    }

    print_statistics(parameters, entries);
    print_parameter_non_zero_coverage(parameters, entries);

    if constexpr (TuneEval::retune_from_zero)
    {
        for (auto& parameter : parameters)
        {
#if TAPERED
            parameter[static_cast<int>(PhaseStages::Midgame)] = static_cast<tune_t>(0);
            parameter[static_cast<int>(PhaseStages::Endgame)] = static_cast<tune_t>(0);
#else
            parameter = static_cast<tune_t>(0);
#endif            
        }
    }

    cout << "Initial parameters:" << endl;
    TuneEval::print_parameters(parameters);

    tune_t K;
    if constexpr (TuneEval::preferred_k <= 0)
    {
        cout << "Finding optimal K..." << endl;
        K = find_optimal_k(thread_pool, entries, parameters);
    }
    else
    {
        cout << "Using predefined K = " << TuneEval::preferred_k << endl;
        K = TuneEval::preferred_k;
    }
    cout << "K = " << K << endl;

    const auto avg_error = get_average_error(thread_pool, entries, parameters, K);
    cout << "Initial error = " << avg_error << endl;

    const auto loop_start = high_resolution_clock::now();
    tune_t learning_rate = TuneEval::initial_learning_rate;
    int32_t max_tune_epoch = TuneEval::max_epoch;
#if TAPERED
    parameters_t momentum(parameters.size(), pair_t{});
    parameters_t velocity(parameters.size(), pair_t{});
#else
    parameters_t momentum(parameters.size(), 0);
    parameters_t velocity(parameters.size(), 0);
#endif
    for (int32_t epoch = 1; epoch < max_tune_epoch; epoch++)
    {
#if TAPERED
        parameters_t gradient(parameters.size(), pair_t{});
#else
        parameters_t gradient(parameters.size(), 0);
#endif

        compute_gradient(thread_pool, gradient, entries, parameters, K);

        constexpr tune_t beta1 = 0.9;
        constexpr tune_t beta2 = 0.999;

        for (int parameter_index = 0; parameter_index < parameters.size(); parameter_index++) {
#if TAPERED
            for (int phase_stage = 0; phase_stage < 2; phase_stage++)
            {
                const tune_t grad = -K / static_cast<tune_t>(400) * gradient[parameter_index][phase_stage] / static_cast<tune_t>(entries.size());
                momentum[parameter_index][phase_stage] = beta1 * momentum[parameter_index][phase_stage] + (1 - beta1) * grad;
                velocity[parameter_index][phase_stage] = beta2 * velocity[parameter_index][phase_stage] + (1 - beta2) * pow(grad, 2);
                parameters[parameter_index][phase_stage] -= learning_rate * momentum[parameter_index][phase_stage] / (static_cast<tune_t>(1e-8) + sqrt(velocity[parameter_index][phase_stage]));
            }
#else
            const tune_t grad = -K / 400.0 * gradient[parameter_index] / static_cast<tune_t>(entries.size());
            momentum[parameter_index] = beta1 * momentum[parameter_index] + (1 - beta1) * grad;
            velocity[parameter_index] = beta2 * velocity[parameter_index] + (1 - beta2) * pow(grad, 2);
            parameters[parameter_index] -= learning_rate * momentum[parameter_index] / (1e-8 + sqrt(velocity[parameter_index]));
#endif

        }
        TuneEval::normalize_pst(parameters);
        if (epoch % 250 == 0)
        {
            const auto elapsed_ms = duration_cast<milliseconds>(high_resolution_clock::now() - loop_start).count();
            const auto epochs_per_second = epoch * 1000.0 / elapsed_ms;
            const tune_t error = get_average_error(thread_pool, entries, parameters, K);
            print_elapsed(start);
            cout << "Epoch " << epoch << " (" << epochs_per_second << " eps), error " << std::setprecision(17) << error << ", LR " << learning_rate << endl;
            if (epoch % 1000 == 0) TuneEval::print_parameters(parameters);
        }

        if (epoch % TuneEval::learning_rate_drop_interval == 0)
        {
            learning_rate *= TuneEval::learning_rate_drop_ratio;
        }
    }

    thread_pool.stop();
    const auto weights_path = find_weights_file();
    TuneEval::write_parameters(parameters, weights_path.string());
    cout << "Wrote complete tuned weights to " << weights_path.string() << endl;
}