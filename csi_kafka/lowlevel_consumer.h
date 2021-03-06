#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include "lowlevel_client.h"

#pragma once
namespace csi {
namespace kafka {
class lowlevel_consumer
{
  public:
  typedef std::function <void(const boost::system::error_code&)>       connect_callback;
  typedef std::function <void(rpc_result<void>)>                       set_offset_callback;
  typedef std::function <void(rpc_result<metadata_response>)>          get_metadata_callback;
  typedef std::function <void(rpc_result<group_coordinator_response>)> get_group_coordinator_callback;
  typedef std::function <void(const boost::system::error_code& ec1, csi::kafka::error_codes ec2, std::shared_ptr<csi::kafka::fetch_response::topic_data::partition_data>)> datastream_callback;
  typedef std::function <void(rpc_result<csi::kafka::fetch_response>)>  fetch_callback;

  enum { MAX_FETCH_SIZE = basic_call_context::MAX_BUFFER_SIZE };

  lowlevel_consumer(boost::asio::io_service& io_service, const std::string& topic, int32_t partition, int32_t rx_timeout, size_t max_packet_size = MAX_FETCH_SIZE);
  ~lowlevel_consumer();
  boost::asio::io_service&               io_service() { return _ios; }
  void                                   connect_async(const broker_address& address, int32_t timeout, connect_callback);
  boost::system::error_code              connect(const broker_address& address, int32_t timeout);

  void                                   connect_async(const boost::asio::ip::tcp::resolver::query& query, int32_t timeout, connect_callback cb);
  boost::system::error_code              connect(const boost::asio::ip::tcp::resolver::query& query, int32_t timeout);
  void                                   close(std::string reason);

  void                                   get_metadata_async(get_metadata_callback cb);
  rpc_result<metadata_response>          get_metadata();

  void                                   get_group_coordinator_async(const std::string& consumer_group, get_group_coordinator_callback cb);
  rpc_result<group_coordinator_response> get_group_coordinator(const std::string& consumer_group);
  void                                   set_offset_time_async(int64_t start_time, set_offset_callback cb);
  rpc_result<void>                       set_offset_time(int64_t start_time);
  void                                   set_offset(int64_t offset);
  int64_t                                get_next_offset() const { return _next_offset; }
  void                                   stream_async(datastream_callback cb);
  void                                   pause();
  void                                   resume();
  void                                   fetch(fetch_callback cb);
  rpc_result<csi::kafka::fetch_response> fetch();


  inline bool                            is_connected() const { return _client.is_connected(); }
  inline bool                            is_connection_in_progress() const { return _client.is_connection_in_progress(); }
  int32_t                                partition() const { return _partition; }
  const std::string&                     topic() const { return _topic; }
  size_t                                 total_rx_bytes() const { return __metrics_total_rx_bytes; }
  size_t                                 total_rx_msg() const { return __metrics_total_rx_msg; }

  protected:
  void _try_fetch();
  void _try_set_offset();

  boost::asio::io_service&        _ios;
  csi::kafka::lowlevel_client     _client;
  const std::string               _topic;
  int32_t                         _rx_timeout;
  bool                            _rx_in_progress;
  datastream_callback             _cb;
  const int32_t                   _partition;
  int64_t                         _next_offset;
  bool                            _transient_failure;
  size_t                          _max_packet_size;
  bool                            _pause_streaming;

  void handle_metrics_timer(const boost::system::error_code& ec);
  boost::asio::deadline_timer	     _metrics_timer;
  boost::posix_time::time_duration _metrics_timeout;
  uint64_t                         __metrics_total_rx_bytes;
  uint64_t                         __metrics_total_rx_msg;
};
}
};