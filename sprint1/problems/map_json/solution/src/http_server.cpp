#include "http_server.h"

namespace http_server {

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
            return ReportError(ec, "read"sv);
        }

        HandleRequest(std::move(request_));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
        using namespace std::literals;

        if (ec) {
            return ReportError(ec, "write"sv);
        }

        if (close) {
            return Close();
        }

        Read();
    }

}  // namespace http_server
