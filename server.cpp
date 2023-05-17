#include <grpc++/grpc++.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#include "proto/highway.grpc.pb.h"
#include "proto/highway.pb.h"

using namespace std;

constexpr int HIGHWAY_COUNT = 50;

struct Highway {
  unsigned int id;
  string name;
  unsigned int vehicle_count;
  unsigned int accidents_count;
};

struct HighwayInfo {
  unsigned int id;
  string name;
  unsigned int vehicle_count;
  unsigned int accidents_count;
};

namespace T {
  constexpr int THREADS_COUNT = 5;

  auto thread_semaphore = counting_semaphore<THREADS_COUNT>(THREADS_COUNT);
  mutex highway_mutexes[HIGHWAY_COUNT];

  void on_transform(HighwayInfo info) {
    cout << info.name << ": " << info.vehicle_count << " vehicles, "
         << info.accidents_count << " accidents" << endl;
  }

  void try_transform(Highway highway) {
    if (!highway_mutexes[highway.id].try_lock()) return;

    if (!thread_semaphore.try_acquire()) {
      highway_mutexes[highway.id].unlock();
      return;
    }

    HighwayInfo info;
    info.name = highway.name;
    info.vehicle_count = highway.vehicle_count;
    info.accidents_count = highway.vehicle_count - 1;

    this_thread::sleep_for(chrono::seconds(1));

    thread_semaphore.release();
    highway_mutexes[highway.id].unlock();

    on_transform(info);
  }
}  // namespace T

namespace E {
  using grpc::Server;
  using grpc::ServerBuilder;
  using grpc::ServerContext;
  using grpc::ServerReader;
  using grpc::Status;

  constexpr int THREADS_COUNT = 2;

  void on_extract(Highway highway) {
    thread transform_thread(T::try_transform, highway);
    transform_thread.detach();
  }

  class HighwayServiceImpl final : public highway::HighwayService::Service {
    Status SendHighway(ServerContext* context,
                       ServerReader<highway::Highway>* reader,
                       highway::Empty* empty) override {
      for (highway::Highway data; reader->Read(&data);) {
        Highway highway;
        highway.id = data.id();
        highway.name = data.name();
        highway.vehicle_count = data.vehicle_count();

        on_extract(highway);
      }

      return Status::OK;
    }
  };

  void listen() {
    string server_address("localhost:50051");
    HighwayServiceImpl service;

    ServerBuilder builder;
    // builder.SetSyncServerOption(ServerBuilder::SyncServerOption::NUM_CQS,
    // THREADS_COUNT);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server listening on " << server_address << endl;

    server->Wait();
  }

}  // namespace E

int main() {
  E::listen();

  return 0;
}
