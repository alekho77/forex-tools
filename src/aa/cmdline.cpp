#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include <boost/filesystem.hpp>

boost::filesystem::path g_srcbin;
boost::filesystem::path g_outbin;
boost::filesystem::path g_config;
double g_pip = 0.0001;
std::string g_algname;
bool g_analyze_mode = false;
bool g_learn_mode = false;
bool g_markup_submode = false;
bool g_training_submode = false;
size_t g_distr_size = 100;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  bool help = false;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", bool_switch(&help), "Show help.")
    ("analyze,a", bool_switch(&g_analyze_mode), "Analyze the algorithm.")
    ("learning,l", bool_switch(&g_learn_mode), "Learn the algorithm.");
  options_description generic_desc("Generic options", 200);
  generic_desc.add_options()
    ("name,n", value<string>(&g_algname)->required()->value_name("name"), "Name of algorithm to analyze or learn.")
    ("config,c", value<string>()->required()->value_name("config")->notifier(
      [](const string& cfgname) { g_config = boost::filesystem::canonical(cfgname); }), "Path to algorithm configuration file.");
  options_description analyze_desc("Analyzing options", 200);
  analyze_desc.add_options()
    ("source,s", value<string>()->required()->value_name("bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to source binary quotes.")
    ("distsize,d", value<size_t>(&g_distr_size)->default_value(100)->value_name("size"), "Number of intervals to build a distribution.");
  options_description learn_desc("Learning options", 200);
  learn_desc.add_options()
    ("markup,m", bool_switch(&g_markup_submode), "Preparation training set.")
    ("training,t", bool_switch(&g_training_submode), "Training the algorithm.");
  options_description markup_desc("Marking-up options", 200);
  markup_desc.add_options()
    ("source,s", value<string>()->required()->value_name("bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to source binary quotes.")
    ("out,o", value<string>()->required()->value_name("[filename]")->implicit_value("")->notifier(
      [](const string& outname) { g_outbin = boost::filesystem::canonical(outname); }), "File to write training set.");
  options_description train_desc("Training options", 200);
  train_desc.add_options()
    ("source,s", value<string>()->required()->value_name("bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to binary training set.");
  options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.");
  const auto list_desc = {basic_desc, generic_desc, analyze_desc, learn_desc, markup_desc, train_desc, additional_desc};
  try {
    store(command_line_parser(argc, argv).options(basic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (help) {
      cout << list_desc;
      return false;
    } else if (g_analyze_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(analyze_desc).add(additional_desc)), vm);
      notify(vm);
    } else if (g_learn_mode) {
      store(command_line_parser(argc, argv).options(learn_desc).allow_unregistered().run(), vm);
      notify(vm);
      if (g_markup_submode) {
        options_description desc;
        store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(learn_desc).add(markup_desc).add(additional_desc)), vm);
        notify(vm);
      } else if (g_training_submode) {
        options_description desc;
        store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(learn_desc).add(train_desc).add(additional_desc)), vm);
        notify(vm);
      } else {
        throw invalid_argument("The submode of learning should be defined (marking-up or training)");
      }
    } else {
      throw invalid_argument("The mode should be defined (analyzing or learning)");
    }
  } catch (const exception& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    cout << list_desc;
    return false;
  }
  return true;
}
