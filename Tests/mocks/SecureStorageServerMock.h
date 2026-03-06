#pragma once

#include <memory>
#include <string>
#include <grpcpp/server.h>

class SecureStorageServiceMock;

class MockSecureStorageServer {
public:
    MockSecureStorageServer(std::unique_ptr<SecureStorageServiceMock> service_mock,
                            const std::string& server_address);

    ~MockSecureStorageServer();

    SecureStorageServiceMock* GetMock();

private:
    std::unique_ptr<SecureStorageServiceMock> service_mock_;
    std::unique_ptr<grpc::Server> server_;
    std::string server_address_;
};
