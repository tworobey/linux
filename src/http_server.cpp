#include "http_server.h"

#include <iostream>

namespace http_server {

    //////////////////////////////////////////////////
    // ERROR
    //////////////////////////////////////////////////

    void ReportError(sys::error_code ec, std::string_view what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }

    //////////////////////////////////////////////////
    // SESSION BASE
    //////////////////////////////////////////////////

    SessionBase::SessionBase(tcp::socket&& socket)
        : stream_(std::move(socket)) {
    }

    void SessionBase::Run() {
        net::dispatch(
            stream_.get_executor(),
            beast::bind_front_handler(
                &SessionBase::Read,
                GetSharedThis()));
    }

    void SessionBase::Read() {
        using namespace std::literals;

        request_ = {};
        stream_.expires_after(30s);

        http::async_read(
            stream_,
            buffer_,
            request_,
            beast::bind_front_handler(
                &SessionBase::OnRead,
                GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, std::size_t) {
        using namespace std::literals;

        if (ec == http::error::end_of_stream) {
            return Close();
        }

        if (ec) {
            ReportError(ec, "read");
            return;
        }

        try {
            HandleRequest(std::move(request_));
        }
        catch (const std::exception& e) {
            std::cerr << "HANDLER EXCEPTION: " << e.what() << std::endl;
        }
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
        using namespace std::literals;

        if (ec) {
            ReportError(ec, "write");
            return;
        }

        if (close) {
            return Close();
        }

        Read();
    }

    void SessionBase::Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        if (ec) {
            ReportError(ec, "shutdown");
        }
    }

}  // namespace http_server
