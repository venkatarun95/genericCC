#include "markoviancc.hh"

#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

// For Remy
double TRAINING_LINK_RATE = 4000000.0/1500.0;

char const* greet()
{
  return "hello, world";
}

BOOST_PYTHON_MODULE(pygenericcc){
  using namespace boost::python;
  class_<MarkovianCC>("MarkovianCC", init<double>())
    .def("get_the_window", &MarkovianCC::get_the_window)
    .def("get_intersend_time", &MarkovianCC::get_intersend_time)
    .def("get_timeout", &MarkovianCC::get_timeout)
    .def("init", &MarkovianCC::init)
    .def("onACK", &MarkovianCC::onACK)
    .def("onTimeout", &MarkovianCC::onTimeout)
    .def("onDupACK", &MarkovianCC::onDupACK)
    .def("onPktSent", &MarkovianCC::onPktSent)
    .def("close", &MarkovianCC::close)
    .def("set_timestamp", &MarkovianCC::set_timestamp)
    .def("set_min_rtt", &MarkovianCC::set_min_rtt)
    .def("interpret_config_str", &MarkovianCC::interpret_config_str);
}
