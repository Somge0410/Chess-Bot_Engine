#include "tuner.h"
#include "tuner.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include "zobrist.h"
using namespace std;
using namespace Tuner;

int main(int argc, char** argv) {
 cout << "Start SPSA tuning? [y/N]: ";
    string confirmation;
    if (!getline(cin, confirmation) || (confirmation != "y" && confirmation != "Y"))
    {
        cout << "SPSA tuning cancelled." << endl;
        return 0;
    }

	Zobrist::initialize_keys();
    vector<DataSource> sources;
    {
        filesystem::path csv_path = "sources.csv";
        if (argc > 1)
        {
            csv_path = argv[1];
        }
        else
        {
            auto directory = filesystem::current_path();
            while (true)
            {
                const auto candidate = directory / "src" / "SPSA Tuning" / "sources.csv";
                if (filesystem::exists(candidate))
                {
                    csv_path = candidate;
                    break;
                }

                const auto parent = directory.parent_path();
                if (parent == directory)
                {
                    break;
                }
                directory = parent;
            }
        }

        cout << "Current working directory: " << filesystem::current_path().string() << endl;

        ifstream csv(csv_path);
        if(!csv)
        {
            cout << "Unable to open data source list " << csv_path.string() << endl;
            return -1;
        }

        string line;
        while (getline(csv, line))
        {
            if(line.empty() || line.starts_with('#'))
            {
                continue;
            }

            DataSource source;
            stringstream ss(line);
            if(!getline(ss, source.path, ','))
            {
                cout << "CSV misformatted" << endl;
                return -1;
            }
            if (filesystem::path(source.path).is_relative())
            {
                source.path = (csv_path.parent_path() / source.path).string();
            }

            string flipped_wdl_str;
            if (!getline(ss, flipped_wdl_str, ','))
            {
                cout << "CSV misformatted" << endl;
                return -1;
            }
            try
            {
                const auto flip_flag = stoul(flipped_wdl_str);
                if (flip_flag > 1)
                {
                    throw std::invalid_argument("WDL flip flag must be 0 or 1");
                }
                source.side_to_move_wdl = flip_flag != 0;
            }
            catch (const std::invalid_argument&)
            {
                cout << flipped_wdl_str << " is not valid for a WDL flip flag";
                return -1;
            }

            string position_limit_str;
            if (!getline(ss, position_limit_str, ','))
            {
                cout << "CSV misformatted" << endl;
                return -1;
            }
            try
            {
                source.position_limit = stoll(position_limit_str);
                if (source.position_limit < 0)
                {
                    throw std::invalid_argument("Position limit must not be negative");
                }
            }
            catch (const std::invalid_argument&)
            {
                cout << position_limit_str << " is not a valid position limit";
                return -1;
            }

            sources.push_back(source);
        }
    }

    if(sources.empty())
    {
        cout << "Data source list is empty";
        return -1;
    }

    run(sources);

    return 0;
}