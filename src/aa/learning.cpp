#include <boost/property_tree/ptree.hpp>

extern bool g_markup_submode;
extern bool g_training_submode;

void Markup(const boost::property_tree::ptree& prop);
void Training(const boost::property_tree::ptree& prop, bool out);

void Learning(const boost::property_tree::ptree& prop, bool out) {
    using namespace std;
    if (g_markup_submode) {
        Markup(prop);
    } else if (g_training_submode) {
        Training(prop, out);
    }
}
