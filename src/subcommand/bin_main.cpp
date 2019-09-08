#include "subcommand.hpp"
#include "odgi.hpp"
#include "args.hxx"
#include "algorithms/bin_path_info.hpp"

namespace odgi {

using namespace odgi::subcommand;

int main_bin(int argc, char** argv) {

    // trick argumentparser to do the right thing with the subcommand
    for (uint64_t i = 1; i < argc-1; ++i) {
        argv[i] = argv[i+1];
    }
    std::string prog_name = "odgi bin";
    argv[0] = (char*)prog_name.c_str();
    --argc;
    
    args::ArgumentParser parser("binning of path information in the graph");
    args::HelpFlag help(parser, "help", "display this help summary", {'h', "help"});
    args::ValueFlag<std::string> dg_out_file(parser, "FILE", "store the graph in this file", {'o', "out"});
    args::ValueFlag<std::string> dg_in_file(parser, "FILE", "load the graph from this file", {'i', "idx"});
    // we now do both coverage and orientation
    //args::Flag coverage(parser, "coverage", "bin average path coverage across the graph", {'c', "coverage"});
    args::ValueFlag<std::string> path_delim(parser, "path-delim", "annotate rows by prefix and suffix of this delimiter", {'D', "path-delim"});
    args::Flag aggregate_delim(parser, "aggregate_delim", "aggregate on path prefix delimiter", {'a', "aggregate-delim"});
    args::ValueFlag<uint64_t> num_bins(parser, "N", "number of bins", {'n', "num-bins"});
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    if (argc==1) {
        std::cout << parser;
        return 1;
    }

    graph_t graph;
    assert(argc > 0);
    std::string infile = args::get(dg_in_file);
    if (infile.size()) {
        if (infile == "-") {
            graph.load(std::cin);
        } else {
            ifstream f(infile.c_str());
            graph.load(f);
            f.close();
        }
    }

    std::string delim = args::get(path_delim);
    bool agg_delim = args::get(aggregate_delim);
    auto get_path_prefix = [&](const std::string& path_name) -> std::string {
        if (agg_delim || delim.empty()) {
            return "NA";
        } else {
            return path_name.substr(0, path_name.find(delim));
        }
    };
    auto get_path_suffix = [&](const std::string& path_name) -> std::string {
        if (agg_delim || delim.empty()) {
            return "NA";
        } else {
            return path_name.substr(path_name.find(delim)+1);
        }
    };

    // our aggregation matrix
    std::vector<std::pair<std::string, std::vector<algorithms::pathinfo_t>>> table;
    algorithms::bin_path_info(graph, (args::get(aggregate_delim) ? args::get(path_delim) : ""), args::get(num_bins), table);
    std::cout << "path.name" << "\t" << "path.prefix" << "\t" << "path.suffix" << "\t" << "bin" << "\t" << "mean.cov" << "\t" << "mean.inv" << "\t" << "mean.pos" << std::endl;
    for (auto& row : table) {
        auto& name = row.first;
        std::string name_prefix = get_path_prefix(name);
        std::string name_suffix = get_path_suffix(name);
        for (uint64_t i = 0; i < row.second.size(); ++i) {
            auto& info = row.second[i];
            if (info.mean_cov) {
                std::cout << name << "\t"
                          << name_prefix << "\t"
                          << name_suffix << "\t"
                          << i+1 << "\t"
                          << info.mean_cov << "\t"
                          << info.mean_inv << "\t"
                          << info.mean_pos << std::endl;
            }
        }
    }

    return 0;
}

static Subcommand odgi_bin("bin", "bin path information across the graph",
                              PIPELINE, 3, main_bin);


}