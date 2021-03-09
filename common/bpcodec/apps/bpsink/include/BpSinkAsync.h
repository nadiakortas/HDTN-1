#ifndef _BP_SINK_ASYNC_H
#define _BP_SINK_ASYNC_H

#include <stdint.h>

//#include "message.hpp"
//#include "paths.hpp"



#include "CircularIndexBufferSingleProducerSingleConsumerConfigurable.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "TcpclBundleSink.h"
#include "StcpBundleSink.h"

namespace hdtn {


class BpSinkAsync {
private:
    BpSinkAsync();
public:
    BpSinkAsync(uint16_t port, bool useTcpcl, bool useStcp, const std::string & thisLocalEidString, const uint32_t extraProcessingTimeMs = 0);  // initialize message buffers
    void Stop();
    ~BpSinkAsync();
    int Init(uint32_t type);
    int Netstart();
    //int send_telemetry();
private:
    void TcpclWholeBundleReadyCallback(boost::shared_ptr<std::vector<uint8_t> > wholeBundleSharedPtr);
    int Process(const std::vector<uint8_t> & rxBuf, const std::size_t messageSize);
    void StartUdpReceive();
    void HandleUdpReceive(const boost::system::error_code & error, std::size_t bytesTransferred, unsigned int writeIndex);
    void PopCbThreadFunc();

    void StartTcpAccept();
    void HandleTcpAccept(boost::shared_ptr<boost::asio::ip::tcp::socket> newTcpSocketPtr, const boost::system::error_code& error);


public:
    uint32_t m_batch;

    uint64_t m_tscTotal;
    int64_t m_rtTotal;
    uint64_t m_totalBytesRx;

    uint64_t m_receivedCount;
    uint64_t m_duplicateCount;
    uint64_t m_seqHval;
    uint64_t m_seqBase;

private:
    const uint16_t m_rxPortUdpOrTcp;
    const bool m_useTcpcl;
    const bool m_useStcp;
    const std::string M_THIS_EID_STRING;
    const uint32_t M_EXTRA_PROCESSING_TIME_MS;

    int m_type;
    boost::asio::io_service m_ioService;
    boost::asio::ip::udp::socket m_udpSocket;
    boost::asio::ip::tcp::acceptor m_tcpAcceptor;

    boost::shared_ptr<TcpclBundleSink> m_tcpclBundleSinkPtr;
    boost::shared_ptr<StcpBundleSink> m_stcpBundleSinkPtr;
    CircularIndexBufferSingleProducerSingleConsumerConfigurable m_circularIndexBuffer;
    std::vector<std::vector<boost::uint8_t> > m_udpReceiveBuffersCbVec;
    std::vector<boost::asio::ip::udp::endpoint> m_remoteEndpointsCbVec;
    std::vector<std::size_t> m_udpReceiveBytesTransferredCbVec;
    boost::condition_variable m_conditionVariableCb;
    boost::shared_ptr<boost::thread> m_threadCbReaderPtr;
    boost::shared_ptr<boost::thread> m_ioServiceThreadPtr;
    volatile bool m_running;
};


}  // namespace hdtn

#endif  //_BP_SINK_ASYNC_H
