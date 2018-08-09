///////////////////////////////////////////////////////////////////////////\file
///
///   Copyright 2018 SINTEF AS
///
///   This Source Code Form is subject to the terms of the Mozilla
///   Public License, v. 2.0. If a copy of the MPL was not distributed
///   with this file, You can obtain one at https://mozilla.org/MPL/2.0/
///
////////////////////////////////////////////////////////////////////////////////

#define BOOST_LOG_DYN_LINK

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <i3ds/power.hpp>
#include <i3ds/communication.hpp>
#include <i3ds/server.hpp>

#include "power_driver.h"

namespace po = boost::program_options;
namespace logging = boost::log;

#define DEFAULT_I2C_PORT "/dev/i2c-2"


namespace i3ds
{

class PetalinuxPower : public Power
{
public:

  typedef std::shared_ptr<PetalinuxPower> Ptr;
  static Ptr Create(NodeID id)
  {
    return std::make_shared<PetalinuxPower>(id);
  }

  PetalinuxPower(NodeID sensor) : Power(sensor)
  {
  }
							       

protected:
  // Handler for channel enable command, must be overloaded.
  void handle_enable_channels(ChannelsEnableService::Data& command) {
    uint16_t mask = 0x0000;
    for (int i=0; i<16; i++) {
      if (command.request.arr[i]) {
	mask |= (1<<i);
      }
    }
    BOOST_LOG_TRIVIAL(info) << "Enabling channel mask:" << boost::format("%1$#x") % mask;
    power_mask_enable(mask);
  }

  // Handler for channel disable command, must be overloaded.
  void handle_disable_channels(ChannelsDisableService::Data& command) {
    uint16_t mask = 0x0000;
    for (int i=0; i<16; i++) {
      if (command.request.arr[i]) {
	mask |= (1<<i);
      }
    }
    BOOST_LOG_TRIVIAL(info) << "Disabling channel mask:" << boost::format("%1$#04x") % mask;
    power_mask_disable(mask);
  }

  void handle_set_channels(ChannelsSetService::Data& command) {
    uint16_t mask = 0x0000;
    for (int i=0; i<16; i++) {
      if (command.request.arr[i]) {
	mask |= (1<<i);
      }
    }
    BOOST_LOG_TRIVIAL(info) << "Setting channel mask:" << boost::format("%1$#x") % mask;
    power_mask_set(mask);
  }
};
} // namespace i3ds

int
main (int argc, char* argv[])
{
  NodeID node_id;
  unsigned int i2c_address, data_address, io_address;
  std::vector<unsigned int> power_ids;
  std::string addr_server;


  po::options_description desc ("Power controller");
  desc.add_options () ("help,h", "Produce this message")
    ("node,n", po::value(&node_id)->default_value(299), "NodeID")
    ("device,d", po::value<std::string> ()->default_value (DEFAULT_I2C_PORT),
     "The i2c device that controls the power")
    ("i2c_address", po::value(&i2c_address)->default_value(0x22), "Address to the i2c register")
    ("i2c_power_address", po::value(&data_address)->default_value(0x04), "Address to the power value register")
    ("i2c_io_address", po::value(&io_address)->default_value(0x8C), "Address to the IO value register")
    ("addr-server,a", po::value(&addr_server)->default_value(""), "Address to the address-server")
    ("verbose,v", "Print verbose output") ("quite,q", "Quiet output");

  po::variables_map vm;
  po::store (po::parse_command_line (argc, argv, desc), vm);
  po::notify (vm);

  std::string device;

  if (vm.count ("help"))
    {
      BOOST_LOG_TRIVIAL(info) << desc;
      return -1;
    }

  if (vm.count ("device"))
    {
      device = vm["device"].as<std::string> ();
      BOOST_LOG_TRIVIAL(info) << "Using device: " << device;
    }
  if (vm.count ("quiet"))
    {
      logging::core::get ()->set_filter (
	  logging::trivial::severity >= logging::trivial::warning);
    }
  else if (!vm.count ("verbose"))
    {
      logging::core::get ()->set_filter (
	  logging::trivial::severity >= logging::trivial::info);
    }

  if (!power_initialize(device.c_str(), i2c_address, data_address, io_address)) {
    BOOST_LOG_TRIVIAL(error) << "Could not initialize power control system";
    return -1;
  }

  i3ds::Context::Ptr context;
  if (addr_server.empty()) {
    context = i3ds::Context::Create();
  } else {
    context = std::make_shared<i3ds::Context>(addr_server);
  }

    
  BOOST_LOG_TRIVIAL(info) << "Creating trigger node with id: " << node_id;
  i3ds::PetalinuxPower::Ptr power = i3ds::PetalinuxPower::Create(node_id);
  i3ds::Server server(context);
  BOOST_LOG_TRIVIAL(info) << "Attaching server.";
  power->Attach(server);
  BOOST_LOG_TRIVIAL(info) << "Starting server.";
  server.Start();
  BOOST_LOG_TRIVIAL(info) << "Server started.";

  while(true) {
    sleep(1000);
  }

  server.Stop();
  power_deinitialize();

  return 0;
}
