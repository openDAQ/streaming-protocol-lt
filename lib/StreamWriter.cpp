#include <cstdint>

#include <boost/asio/buffer.hpp>

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/StreamWriter.h"

namespace daq::streaming_protocol{

StreamWriter::StreamWriter(std::shared_ptr<daq::stream::Stream> stream)
    : m_stream(stream)
{
}

std::string StreamWriter::id() const
{
    return m_stream->endPointUrl();
}

int StreamWriter::writeMetaInformation(unsigned int signalNumber, const nlohmann::json &data)
{
    std::vector < uint8_t > msgpack = nlohmann::json::to_msgpack(data);
    return writeMsgPackMetaInformation(signalNumber, msgpack);
}

int StreamWriter::writeMsgPackMetaInformation(unsigned int signalNumber, const std::vector<uint8_t>& data)
{
    daq::stream::ConstBufferVector buffers(3);

    // used by openDAQ streaming which both use little endian!
    static const uint32_t littleMetaType = METAINFORMATION_MSGPACK;
    size_t headerSize = createTransportHeader(TYPE_METAINFORMATION, signalNumber, data.size() + sizeof (littleMetaType));
    /// 3 parts: transport header (evtl. with optional additional length), meta information type, meta information payload
    buffers[0] = boost::asio::const_buffer(&m_transportHeaderBuffer[0], headerSize);
    buffers[1] = boost::asio::const_buffer(reinterpret_cast< const char*>(&littleMetaType), sizeof (littleMetaType));
    buffers[2] = boost::asio::const_buffer(&data[0], data.size());

    boost::system::error_code ec;
    return static_cast <int> (m_stream->write(buffers, ec));
}

int StreamWriter::writeSignalData(unsigned int signalNumber, const void *pData, size_t length)
{
    daq::stream::ConstBufferVector buffers(2);
    size_t headerSize = createTransportHeader(TYPE_SIGNALDATA, signalNumber, length);
    /// 2 parts: transport header (evtl. with optional additional length), signal data payload
    buffers[0] = boost::asio::const_buffer(&m_transportHeaderBuffer[0], headerSize);
    buffers[1] = boost::asio::const_buffer(pData, length);

    boost::system::error_code ec;
    return static_cast <int> (m_stream->write(buffers, ec));
}

size_t StreamWriter::createTransportHeader(TransportType type, unsigned int signalNumber, size_t size)
{
    // lower 20 bit are signal id
    uint32_t header = signalNumber;
    header |= static_cast < uint32_t > (type << TYPE_SHIFT);
    if (size<=0xff) { // 8 bits for the size
        header |= size << SIZE_SHIFT;
        m_transportHeaderBuffer[0] = header;
        // size of the transport header
        return sizeof(uint32_t);
    } else {
        m_transportHeaderBuffer[0] = header;
        m_transportHeaderBuffer[1] = static_cast < uint32_t > (size);
        // size of the transport header + size of additional length field
        return 2*sizeof(uint32_t);
    }
}
}
