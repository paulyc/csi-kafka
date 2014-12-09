#include "producer.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

namespace csi
{
    namespace kafka
    {
        producer::producer(boost::asio::io_service& io_service, const boost::asio::ip::tcp::resolver::query& query, const std::string& topic, int32_t partition) :
            _ios(io_service),
            _client(io_service, query),
            _topic_name(topic),
            _partition_id(partition)
        {
        }

        void producer::connect_async(connect_callback cb)
        {
            _client.connect_async(cb);
        }

        boost::system::error_code producer::connect()
        {
            return _client.connect();
        }

        void producer::send_async(int32_t required_acks, int32_t timeout, const std::vector<basic_message>& v, int32_t correlation_id, callback cb)
        {
            _client.perform_async(csi::kafka::create_produce_request(_topic_name, _partition_id, required_acks, timeout, v, correlation_id), [this, cb, correlation_id](csi::kafka::error_codes error, csi::kafka::basic_call_context::handle handle)
            {
                // find the error code thast belongs to this topic
                std::shared_ptr<produce_response>  result = parse_produce_response(handle);

                assert(correlation_id == result->correlation_id);

                //for now only one topic / many partitions
                assert(result->topics.size() == 1);
                for (std::vector<produce_response::topic_data>::const_iterator i = result->topics.begin(); i != result->topics.end(); ++i)
                {
                    if (i->topic_name != _topic_name)
                    {
                        std::cerr << "unexpected topic name " << i->topic_name << std::endl;
                        break;
                    }

                    // for now our call sends only one partition - so this will be a vector of 1
                    assert(i->partitions.size() == 1);
                    for (std::vector<produce_response::topic_data::partition_data>::const_iterator j = i->partitions.begin(); j != i->partitions.end(); ++j)
                    {
                        if (j->error_code())
                        {
                            std::cerr << "produce_response: topic:" << i->topic_name << ", partition:" << j->partition_id << ", error:"  << csi::kafka::to_string(j->error_code()) << std::endl;
                            error = j->error_code();
                        }
                    }
                }
                cb(error, result);
            });
        }

        /*
        std::shared_ptr<csi::kafka::produce_response> producer::send_sync(int32_t required_acks, int32_t timeout, const std::vector<basic_message>& v, int32_t correlation_id, callback cb)
        {
            auto handle = _client.perform_sync(csi::kafka::create_produce_request(_topic_name, _partition_id, required_acks, timeout, v, correlation_id), NULL);
            auto result = parse_produce_response(handle);
            if (cb)
                cb(0, result);
            return result;
        }
        */

        
        //void producer::_on_retry_timer(const boost::system::error_code& ec)
        //{
        //    if (!ec)
        //    {
        //        if (_state == CONNETING_TO_CLUSTER)
        //        {
        //        }
        //        /*
        //        else if (_state == CONNECTING_TO_PARTION_LEADER)
        //        {

        //        }
        //        */
        //    }
        //}

        //void producer::_on_cluster_connect(const boost::system::error_code& ec)
        //{
        //    if (ec)
        //    {
        //        // retry in one second...
        //        // set timer
        //    }

        //    if (!ec)
        //    {
        //        _state = GETTING_METADATA;
        //        _client.perform_async(csi::kafka::create_metadata_request({ _topic_name }, 0), boost::bind(&producer::_on_metadata_request, this, _1, _2));
        //    }

        //}

        //void producer::_on_metadata_request(csi::kafka::error_codes ec, csi::kafka::basic_call_context::handle handle)
        //{
        //    auto response = csi::kafka::parse_metadata_response(handle);
        //    _brokers = response->brokers;

        //    // add the brokers to the known host in the cluster TODO

        //    _partition_leader = -1;
        //    for (std::vector<csi::kafka::metadata_response::topic_data>::const_iterator i = response->topics.begin(); i != response->topics.end(); ++i)
        //    {
        //        if (i->topic_name == _topic_name)
        //        {
        //            if (i->error_code)
        //            {
        //                std::cerr << "topic error from metadata request " << i->error_code << std::endl; // should we retry here??
        //            }
        //            else
        //            {
        //                for (std::vector<csi::kafka::metadata_response::topic_data::partition_data>::const_iterator j = (*i).partitions.begin(); j != (*i).partitions.end(); ++j)
        //                {
        //                    if (j->partition_id == _partition_id)
        //                    {
        //                        if (j->error_code != 0)
        //                        {
        //                            std::cerr << "partition error from metadata request " << j->error_code << std::endl; // should we retry here??
        //                        }
        //                        else
        //                        {
        //                            _partition_leader = j->leader;
        //                        }
        //                    }
        //                }
        //            }
        //        }
        //    }

        //    // not in use right now since we only have one server - difficult to test and reslies on stuff in client that we have not yet written...

        //    // we found our partition leader - let's connect to that node instead.
        //    //if (_partition_leader >= 0)
        //    //{
        //    //    for (std::vector<csi::kafka::broker_data>::const_iterator i = response->brokers.begin(); i != response->brokers.end(); ++i)
        //    //    {
        //    //        if (i->node_id == _partition_leader)
        //    //        {
        //    //            _client.connect();
        //    // CONNECTING_TO_PARTION_LEADER
        //    //        }
        //    //    }
        //    //}
        //    _state = CONNECTING_TO_PARTION_LEADER;
        //    boost::system::error_code ec2;
        //    _ios.post(boost::bind(&producer::_on_leader_connect, this, ec2));
        //}

        //void producer::_on_leader_connect(const boost::system::error_code& ec)
        //{
        //    if (ec)
        //    {
        //        // retry in one second...
        //        // set timer
        //    }

        //    if (!ec)
        //    {
        //        _state = READY;
        //        //_client.perform_async(csi::kafka::create_simple_offset_request(_topic_name, _partition_id, csi::kafka::earliest_available_offset, 10, 0), boost::bind(&producer::_on_offset_request, this, _1, _2));
        //    }
        //}
    } // kafka
}; // csi
