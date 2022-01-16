

#include <etcpal/cpp/common.h>

#include <sACN/cpp/common.h>
#include <sACN/cpp/source.h>
#include <sACN/cpp/source_detector.h>
#include <sACN/cpp/receiver.h>
#include <sACN/cpp/dmx_merger.h>
#include <iostream>
#include <vector>
#include <thread>

using namespace std;
// Implement the callback functions by inheriting sacn::Receiver::NotifyHandler:
class MyNotifyHandler : public sacn::Receiver::NotifyHandler
{
  // Required callbacks that must be implemented:
  void HandleUniverseData(sacn::Receiver::Handle receiver_handle, const etcpal::SockAddr& source_addr,
                          const SacnRemoteSource& source_info, const SacnRecvUniverseData& universe_data) override;
  void HandleSourcesLost(sacn::Receiver::Handle handle, uint16_t universe, const std::vector<SacnLostSource>& lost_sources) override;
 
  // Optional callbacks - these don't have to be a part of MyNotifyHandler:
  //void HandleSamplingPeriodStarted(sacn::Receiver::Handle handle, uint16_t universe) override;
  void HandleSamplingPeriodEnded(sacn::Receiver::Handle handle, uint16_t universe) override;
  void HandleSourcePapLost(sacn::Receiver::Handle handle, uint16_t universe, const SacnRemoteSource& source) override;
  void HandleSourceLimitExceeded(sacn::Receiver::Handle handle, uint16_t universe) override;
};
void MyNotifyHandler::HandleSourceLimitExceeded(sacn::Receiver::Handle handle, uint16_t universe)
{
  // Handle the condition in an application-defined way. Maybe log it?
}
void MyNotifyHandler::HandleSourcesLost(sacn::Receiver::Handle handle, uint16_t universe,
                                        const std::vector<SacnLostSource>& lost_sources)
{
  // You might not normally print a message on this condition, but this is just to demonstrate
  // the fields available:
  std::cout << "The following sources have gone offline:\n";
  for(const SacnLostSource& src : lost_sources)
  {
    std::cout << "CID: " << etcpal::Uuid(src.cid).ToString() << "\tName: " << src.name << "\tTerminated: " 
              << src.terminated << "\n";
 
    // Remove the source from your state tracking...
  }
}
void MyNotifyHandler::HandleSourcePapLost(sacn::Receiver::Handle handle, uint16_t universe, const SacnRemoteSource& source)
{
  // Revert to using the per-packet priority value to resolve priorities for this universe.
}
 void MyNotifyHandler::HandleSamplingPeriodEnded(sacn::Receiver::Handle handle, uint16_t universe)
{
  // Apply universe data as needed...
}
 void MyNotifyHandler::HandleUniverseData(sacn::Receiver::Handle receiver_handle, const etcpal::SockAddr& source_addr,
                                         const SacnRemoteSource& source_info, const SacnRecvUniverseData& universe_data)
{
  // You wouldn't normally print a message on each sACN update, but this is just to demonstrate the
  // header fields available:
  //*
  cout << "Got sACN update from source " << etcpal::Uuid(source_info.cid).ToString() << " (address " 
            << source_addr.ToString() << ", name " << source_info.name << ") on universe "
            << universe_data.universe_id << ", priority " << universe_data.priority << ", start code "
            << universe_data.start_code 
            <<", Slot - Start  : " << universe_data.slot_range.start_address 
            <<", Slot - End  : " << universe_data.slot_range.address_count 
            << ", Value : " << universe_data.values << endl; 
 //*/
  cout << (int) universe_data.values[0] << "    " <<  (int) universe_data.values[1] <<"    " << (int) universe_data.values[2] << "    " << (int) universe_data.values[3] << endl; 
  if (universe_data.is_sampling)
    cout << " (during the sampling period)\n";
  else
    cout << "\n";
 
  // Example for an sACN-enabled fixture...
  /*

  if ((universe_data.start_code == 0x00) && (universe_data.slot_range.start_address == my_start_addr) &&
      (universe_data.slot_range.address_count == MY_DMX_FOOTPRINT))
  {
    // values[0] will always be the level of the first slot of the footprint
    memcpy(my_data_buf, universe_data.values, MY_DMX_FOOTPRINT);
    // Act on the data somehow
  }*/
}

int main(){
  // Now to set up a receiver:
  sacn::Receiver::Settings config(1); // Instantiate config & listen on universe 1
  sacn::Receiver receiver ; // Instantiate a receiver
  MyNotifyHandler my_notify_handler;
  // During startup:
  EtcPalLogParams log_params = ETCPAL_LOG_PARAMS_INIT;
  // Initialize log_params...
  
  sacn_init(&log_params, NULL);

  
 //   std::thread t1(sacn::Receiver::Startup,&receiver,&config, &my_notify_handler);
  int counter = 0;
  receiver.Startup(config, my_notify_handler);
  //std::vector<SacnMcastInterface> my_netints;  // Assuming my_netints is initialized by the application...
  //SacnMcastInterface curInt ;
  //EtcPalMcastNetintId mcast;
  
  //receiver.Startup(config, my_notify_handler, my_netints);

  do{
    
    // Get the universe currently being listened to
    /*
    auto result = receiver.GetUniverse();
    if (result)
    {
      // Change the universe to listen to
      uint16_t new_universe = *result + 1;
      cout << "Got a new Universe " << new_universe << endl;
      receiver.ChangeUniverse(new_universe);
      
    }
    }↨:!#=%239jJ
    */
    this_thread::sleep_for(1s);
    system("cls");
    
    cout << "CH1 .. CH2 .. CH3 .. CH4"<< endl;
    
  }while(true);
  receiver.Shutdown();
  sacn_deinit();
  // Or do this if Startup is being called within the NotifyHandler-derived class:
  //receiver.Startup(config, *this);
  // Or do this to specify custom interfaces for the receiver to use:
  
  // To destroy the receiver when you're done with it:
  
  //t1.join();
}
