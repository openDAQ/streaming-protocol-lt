/*
 * Copyright 2022-2023 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fstream>
#include <functional>


#include "stream/Stream.hpp"

#include "streaming_protocol/ProducerSession.hpp"
#include "streaming_protocol/Server.hpp"

namespace daq::streaming_protocol {
    
    Server::Server(boost::asio::io_context& readerIoContext, uint16_t wsDataPort, LogCallback logCb)
        : m_readerIoContext(readerIoContext)
        , m_server(m_readerIoContext, std::bind(&Server::createSession, this, std::placeholders::_1), wsDataPort)
        , logCallback(logCb)
    {
    }
    
    Server::~Server()
    {
    }    
    
    int Server::start()
    {
        STREAMING_PROTOCOL_LOG_I("Starting");
        m_server.start();
        return 0;
    }
    
    void Server::stop()
    {
        STREAMING_PROTOCOL_LOG_I("Stopping");
        m_server.stop();
        {
            std::lock_guard < std::mutex > lock(m_sessionsMtx);
            for (auto& iter: m_sessions) {
                // check whether the weak pointer is still valid. Own it for some time to stop the session and release again
                if (auto session = iter.second.lock()) {
                    session->stop();
                }
            }
            m_sessions.clear();
        }
    }

    size_t Server::sessionCount() const
    {
        return m_sessions.size();
    }
    
    void Server::createSession(std::shared_ptr<stream::Stream> newStream)
    {
        std::string sessionId = newStream->endPointUrl();
        nlohmann::json commandInterfaces; // empty for now. Will be filled when ControlServer is is place.
        auto newSession = std::make_shared<ProducerSession>(newStream, commandInterfaces, logCallback);

        {
            std::lock_guard < std::mutex > lock(m_sessionsMtx);
            m_sessions[sessionId] = newSession;
        }

        // send meta information with all available signal ids
        if (!m_availableSignals.empty()) {
            //newSession->addSignals(m_availableSignals);
        }

        // Set callback to be executed upon disconnect of consumer (client)
        newSession->start(std::bind(&Server::removeSessionCb, this, sessionId));
    }
    
    void Server::updateAvailableSignals(const SignalIds& removedSignals, const SignalIds& addedSignals)
    {
        if (removedSignals.empty() && addedSignals.empty()) {
            // no relevant changes!
            return;
        }
        
        std::lock_guard < std::mutex > lock(m_sessionsMtx);
        for (auto& iter: m_sessions) {
            // send meta information informing about available/unavailable signals
            
            // check whether the weak pointer is still valid. Own it for some time to do the work and release again
            if (auto sharedPointer = iter.second.lock()) {
                if (!addedSignals.empty()) {
                    //sharedPointer->addSignals(addedSignals);
                }
                
                if (!removedSignals.empty()) {
                    sharedPointer->removeSignals(removedSignals);
                }
            } else {
                m_sessions.erase(iter.first);
            }
        }
    }

    void Server::removeSessionCb(const std::string &sessionId)
    {
        m_sessions.erase(sessionId);
    }
}
