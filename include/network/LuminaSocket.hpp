#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <utility>
#include <system_error>

namespace lumina {
    class LuminaSocket{
    public:
        explicit LuminaSocket(uint16_t port) {
            // IPv4, TCP(SOCK_STREAM) socket creation
            fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (fd_ < 0) {
                throw std::system_error(errno, std::generic_category(), "Failed to create socket");
            }

            int opt = 1;
            if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                close();
                throw std::system_error(errno, std::generic_category(), "Failed to setsockopt(SO_REUSEADDR)");
            }

            // 넌블로킹(O_NONBLOCK) kernel flag 주입
            set_nonblocking();

            // address binding
            struct sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
                close();
                throw std::system_error(errno, std::generic_category(), "Failed to bind socket");
            }

            // kernel backlog queue open
            if (::listen(fd_, SOMAXCONN) < 0) {
                close();
                throw std::system_error(errno, std::generic_category(), "Failed to listen on socket");
            }
        }

        ~LuminaSocket() {
            close();
        }

        // Shared-Nothing & RAII 보호: 소켓 자원의 이중 해재(double free) 방지
        // copy constructor prohibited (LuminaSocket s2 = s1; is not allowed)
        LuminaSocket(const LuminaSocket& other) = delete;
        // assignment operator prohibited (s2 = s1; is not allowed)
        LuminaSocket& operator=(const LuminaSocket& other) = delete;

        // Move Semantics: std::exchange 를 통해 이전 객체의 FD를 -1로 밀어버리고 소유권 전권 이전
        LuminaSocket(LuminaSocket&& other) noexcept
            : fd_(std::exchange(other.fd_, -1)) {}

        LuminaSocket& operator=(LuminaSocket&& other) noexcept {
            if (this != &other) {
                close();
                fd_ = std::exchange(other.fd_, -1);
            }
            return *this;
        }

        [[nodiscard]] int get_fd() const noexcept { return fd_; }

    private:
        void set_nonblocking() {
            int flags = ::fcntl(fd_, F_GETFL, 0);
            if (flags < 0) {
                close();
                throw std::system_error(errno, std::generic_category(), "Failed to fcntl(F_GETFL)");
            }
            if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
                close();
                throw std::system_error(errno, std::generic_category(), "Failed to fcntl(F_SETFL O_NONBLOCK)");
            }
        }

        void close() noexcept {
            if (fd_ >= 0) {
                ::close(fd_);
                fd_ = -1;
            }
        }

        int fd_{-1};
    };

}
