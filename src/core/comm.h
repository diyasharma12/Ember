#pragma once

namespace core {

// Minimal read/write abstraction, mirroring Go's io.ReadWriter interface that
// FDcomm implemented.
class IComm {
   public:
    virtual ~IComm() = default;
    virtual long Read(char* buf, long count) = 0;
    virtual long Write(const char* buf, long count) = 0;
};

class FDComm : public IComm {
   public:
    explicit FDComm(int fd) : fd_(fd) {}

    long Read(char* buf, long count) override;
    long Write(const char* buf, long count) override;

    int Fd() const { return fd_; }

   private:
    int fd_;
};

}  // namespace core
