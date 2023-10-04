#include <functional>
#include <memory>
#include <thread>

#include "streaming_protocol/Logging.hpp"

#include "signalgenerator.h"
#include "worker.h"

using namespace daq::streaming_protocol;

Worker::Worker(std::shared_ptr<daq::stream::Stream> stream)
    : m_signalGenerator(std::chrono::system_clock::now(), stream)
    , logCallback(daq::streaming_protocol::Logging::logCallback())
{
}

Worker::~Worker()
{
    STREAMING_PROTOCOL_LOG_D("Worker closing down...");
}

void Worker::start(const std::string &configFile)
{
    m_thread = std::thread(std::bind(&Worker::threadFunction, shared_from_this(), configFile));
    m_thread.detach();
}

void Worker::threadFunction(const std::string &configFileName)
{
    if (configFileName.empty()) {
        static const std::chrono::seconds duration(10);
        static const std::chrono::milliseconds processPeriod(100);
        daq::streaming_protocol::siggen::addSignals(m_signalGenerator);
        m_signalGenerator.configureTimes(processPeriod, duration);
    } else {
        if (m_signalGenerator.configureFromFile(configFileName)) {
            return;
        }
    }
    m_signalGenerator.start(true);
}
