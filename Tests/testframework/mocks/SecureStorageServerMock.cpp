#include "SecureStorageServerMock.h"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <iostream>
#include "SecureStorageServiceMock.h"

MockSecureStorageServer::MockSecureStorageServer(
    std::unique_ptr<SecureStorageServiceMock> service_mock,
    const std::string& server_address)
    : service_mock_(std::move(service_mock)),
      server_address_(server_address)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_mock_.get());
    server_ = builder.BuildAndStart();
    std::cout << "Mock gRPC server listening on " << server_address_ << std::endl;
}

MockSecureStorageServer::~MockSecureStorageServer() {
    if (server_) {
        std::cout << "Mock gRPC server shutting down."<< std::endl;
        server_->Shutdown();
        server_->Wait();
    }
}

SecureStorageServiceMock* MockSecureStorageServer::GetMock() {
    return service_mock_.get();
}
