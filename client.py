import grpc
from proto import highway_pb2 as pb2, highway_pb2_grpc as pb2_grpc
from time import sleep
from random import randint, random

if __name__ == "__main__":
    channel = grpc.insecure_channel("localhost:50051")
    stub = pb2_grpc.HighwayServiceStub(channel)

    while True:
        highways = []

        for i in range(randint(1, 50)):
            hid = randint(0, 50)

            highways.append(
                pb2.Highway(
                    id=hid,
                    name=f"BR-1{hid:02d}",
                    vehicle_count=randint(0, 100),
                )
            )

        stub.SendHighway(iter(highways))

        print(f"Sent {len(highways):02d} highways")
        sleep(random() * 0.1)
