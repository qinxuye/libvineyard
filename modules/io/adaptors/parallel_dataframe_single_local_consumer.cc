/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>
#include <string>

#include "basic/stream/dataframe_stream.h"
#include "basic/stream/parallel_stream.h"
#include "client/client.h"
#include "io/io/local_io_adaptor.h"

using namespace vineyard;  // NOLINT(build/namespaces)

int main(int argc, const char** argv) {
  if (argc < 3) {
    printf(
        "usage ./parallel_batch_single_local_consumer <ipc_socket> <stream_id> "
        "<ofile>");
    return 1;
  }

  std::string ipc_socket = std::string(argv[1]);
  ObjectID stream_id = VYObjectIDFromString(argv[2]);
  std::string ofile = std::string(argv[3]);

  Client client;
  VINEYARD_CHECK_OK(client.Connect(ipc_socket));
  LOG(INFO) << "Connected to IPCServer: " << ipc_socket;

  auto s = std::dynamic_pointer_cast<ParallelStream<DataframeStream>>(
      client.GetObject(stream_id));
  LOG(INFO) << "Got parallel stream " << s->id();

  std::unique_ptr<LocalIOAdaptor> local_io_adaptor(
      new LocalIOAdaptor(ofile.c_str()));
  VINEYARD_CHECK_OK(local_io_adaptor->Open("w"));

  for (size_t i = 0; i < s->GetStreamSize(); ++i) {
    auto ls = s->GetStream(i);
    if (!ls->IsLocal()) {
      continue;
    }
    LOG(INFO) << "Got dataframe stream " << ls->id() << " " << i;
    auto reader = ls->OpenReader(client);
    std::string line;
    while (reader->ReadLine(line).ok()) {
      VINEYARD_CHECK_OK(local_io_adaptor->WriteLine(line));
    }
  }

  VINEYARD_CHECK_OK(local_io_adaptor->Close());
  local_io_adaptor->Finalize();

  return 0;
}
