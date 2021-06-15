/*  =========================================================================
    fty-msgbus-cli - description

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty-msgbus-cli -
@discuss
@end
*/

#include "fty/messagebus/MsgBusFactory.hpp"
#include "fty/messagebus/mlm/MsgBusMalamute.hpp"
#include "fty/messagebus/mlm/MsgBusMlmMessage.hpp"
#include "fty/messagebus/utils/MsgBusHelper.hpp"

#include <condition_variable>
#include <csignal>
#include <fty_log.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unistd.h>

// Signal handler stuff.

using namespace fty::messagebus;
using namespace fty::messagebus::mlm;
using Message = fty::messagebus::mlm::MlmMessage;
using MessageBus = fty::messagebus::IMessageBus<Message>;

volatile bool g_exit = false;
std::condition_variable g_cv;
std::mutex g_mutex;

void sigHandler(int)
{
  g_exit = true;
  g_cv.notify_one();
}

void setSignalHandler()
{
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = sigHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, nullptr);
}

// Command line parameters.

std::string endpoint, type, subject, topic, queue, destination, timeout = "5";
std::string clientName;
bool doMetadata = true;

void sendRequest(MessageBus* msgbus, int argc, char** argv);
void request(MessageBus* msgbus, int argc, char** argv);
void receive(MessageBus* msgbus, int argc, char** argv);
void subscribe(MessageBus* msgbus, int argc, char** argv);
void publish(MessageBus* msgbus, int argc, char** argv);

struct progAction
{
  std::string arguments;
  std::string help;
  void (*fn)(MessageBus*, int, char**);
};

const std::map<std::string, progAction> actions = {
  {"sendRequest", {"[userData]", "send a request with payload", sendRequest}},
  {"request", {"[userData]", "send a request with payload and wait for response", request}},
  {"receive", {"", "listen on a queue and dump out received messages", receive}},
  {"subscribe", {"", "subscribe on a topic and dump out received messages", subscribe}},
  {"publish", {"", "publish a message on a topic", publish}},
};

const std::map<std::string, std::function<MessageBus*()>> busTypes = {
  {"malamute", []() -> MessageBus* { return MessageBusFactory::createMlmMsgBus(endpoint, clientName); }},
};

void dumpMessage(const MlmMessage& msg)
{
  std::stringstream buffer;
  buffer << "--------------------------------------------------------------------------------\n";
  for (const auto& metadata : msg.metaData())
  {
    buffer << "* " << metadata.first << ": " << metadata.second << "\n";
  }
  int cpt = 0;
  for (const auto& data : msg.userData())
  {
    buffer << std::to_string(cpt) << ": " << data << "\n";
    cpt++;
  }
  log_info(buffer.str().c_str());
}

void receive(MessageBus* msgbus, int /*argc*/, char** /*argv*/)
{
  msgbus->receive(queue, [](const MlmMessage& msg) { dumpMessage(msg); });

  // Wait until interrupt.
  setSignalHandler();
  std::unique_lock<std::mutex> lock(g_mutex);
  g_cv.wait(lock, [] { return g_exit; });
}

void subscribe(MessageBus* msgbus, int /*argc*/, char** /*argv*/)
{
  msgbus->subscribe(topic, [](const MlmMessage& msg) { dumpMessage(msg); });

  // Wait until interrupt.
  setSignalHandler();
  std::unique_lock<std::mutex> lock(g_mutex);
  g_cv.wait(lock, [] { return g_exit; });
}

void sendRequest(MessageBus* msgbus, int /*argc*/, char** argv)
{
  MlmMessage msg;

  // Build message metadata.
  if (doMetadata)
  {
    msg.metaData() =
      {
        {FROM, clientName},
        {REPLY_TO, clientName},
        {SUBJECT, subject},
        {CORRELATION_ID, utils::generateUuid()},
        {TO, destination},
        {TIMEOUT, timeout},
      };
  }

  // Build message payload.
  while (*argv)
  {
    msg.userData().emplace_back(*argv++);
  }

  dumpMessage(msg);
  msgbus->sendRequest(queue, msg);
}

void request(MessageBus* msgbus, int /*argc*/, char** argv)
{
  Message msg;

  // Build message metadata.
  if (doMetadata)
  {
    msg.metaData() =
      {
        {FROM, clientName},
        {REPLY_TO, clientName},
        {SUBJECT, subject},
        {CORRELATION_ID, utils::generateUuid()},
        {TO, destination},
        {TIMEOUT, timeout},
      };
  }

  // Build message payload.
  while (*argv)
  {
    msg.userData().emplace_back(*argv++);
  }

  dumpMessage(msg);
  try
  {
    dumpMessage(msgbus->request(queue, msg, stoi(timeout)));
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Caught exception: " << ex.what() << std::endl;
  }
}

void publish(MessageBus* msgbus, int /*argc*/, char** argv)
{
  Message msg;

  // Build message metadata.
  if (doMetadata)
  {
    msg.metaData() =
      {
        {SUBJECT, subject},
      };
  }

  // Build message payload.
  while (*argv)
  {
    msg.userData().emplace_back(*argv++);
  }

  dumpMessage(msg);
  msgbus->publish(topic, msg);
}

[[noreturn]] void usage()
{
  std::cerr << "Usage: fty-msgbus-cli [options] action ..." << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "\t-h                      this information" << std::endl;
  std::cerr << "\t-e endpoint             endpoint to connect to" << std::endl;
  std::cerr << "\t-s subject              subject of message" << std::endl;
  std::cerr << "\t-t topic                topic to use" << std::endl;
  std::cerr << "\t-T timeout              timeout to use" << std::endl;
  std::cerr << "\t-q queue                queue to use" << std::endl;
  std::cerr << "\t-d destination          destination (messagebus::Message::TO metadata)" << std::endl;
  std::cerr << "\t-x                      send message with no metadata (for old-school Malamute)" << std::endl;

  std::cerr << "\t-i type                 message bus type (";
  for (auto it = busTypes.begin(); it != busTypes.end(); it++)
  {
    if (it != busTypes.begin())
    {
      std::cerr << ", ";
    }
    std::cerr << it->first;
  }
  std::cerr << ")" << std::endl;

  std::cerr << "\nActions:" << std::endl;
  for (const auto& i : actions)
  {
    size_t left = (24 - i.first.length() - i.second.arguments.length() - 2);
    std::cerr << "\t" << i.first << " " << i.second.arguments << std::string(left + 1, ' ') << i.second.help << std::endl;
  }

  exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
  endpoint = DEFAULT_MLM_END_POINT;
  clientName = utils::getClientId("fty-msgbus-cli");
  type = "malamute";

  int c;
  while ((c = getopt(argc, argv, "he:s:t:T:q:d:xi:")) != -1)
  {
    switch (c)
    {
      case 'h':
        usage();
      case 'e':
        endpoint = optarg;
        break;
      case 's':
        subject = optarg;
        break;
      case 't':
        topic = optarg;
        break;
      case 'T':
        timeout = optarg;
        break;
      case 'q':
        queue = optarg;
        break;
      case 'd':
        destination = optarg;
        break;
      case 'x':
        doMetadata = false;
        break;
      case 'i':
        type = optarg;
        break;
      case ':':
        std::cerr << "Option -" << static_cast<char>(optopt) << " requires an operand" << std::endl;
        usage();
      case '?':
        std::cerr << "Unrecognized option: -" << static_cast<char>(optopt) << std::endl;
        usage();
    }
  }

  // Find bus.
  auto busIt = busTypes.find(type);
  if (busIt == busTypes.end())
  {
    std::cerr << "Unknown message bus type '" << type << "'" << std::endl;
    usage();
  }

  // Find action.
  if (optind == argc)
  {
    std::cerr << "Action missing from arguments" << std::endl;
    usage();
  }
  auto actionIt = actions.find(argv[optind]);
  if (actionIt == actions.end())
  {
    std::cerr << "Unknown action '" << argv[optind] << "'" << std::endl;
    usage();
  }

  // Do the requested work.
  auto msgBus = std::unique_ptr<MessageBus>(busIt->second());
  msgBus->connect();
  actionIt->second.fn(msgBus.get(), argc - optind - 1, argv + optind + 1);

  return 0;
}
