#include <boost/property_tree/ptree.hpp>

extern bool g_markup_submode;
extern bool g_training_submode;

void Markup(const boost::property_tree::ptree& /*prop*/);

void Learning(const boost::property_tree::ptree& prop) {
  using namespace std;
  if (g_markup_submode) {
    Markup(prop);
  }
}