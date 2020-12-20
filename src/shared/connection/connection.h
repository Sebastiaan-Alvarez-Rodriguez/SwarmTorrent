#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


// Simple tutorial to get started
//  https://www.geeksforgeeks.org/socket-programming-cc/
//  https://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm

#include "shared/connection/meta/type.h"


class Connection {
public:
    Connection(ConnectionType type, uint16_t sourcePort, bool blockmode, bool reusemode, unsigned sendTimeout, unsigned recvTimeout) : type(type), sourcePort(sourcePort), blockmode(blockmode), reusemode(reusemode), sendTimeout(sendTimeout), recvTimeout(recvTimeout) {}
    ~Connection() = default;

    /** Returns type of connection we use */
    virtual const ConnectionType& get_type() const {return type;}

    inline virtual void print(std::ostream& stream) const = 0;

    inline virtual uint16_t getSourcePort() const {return sourcePort;}


    inline virtual bool setBlocking(bool blockmode) = 0;

    inline virtual bool isBlocking() const {
        return blockmode;
    }

    inline virtual bool isReuse() const {
        return reusemode;
    }

    inline virtual unsigned getSendTimeout() const {
        return sendTimeout;
    }

    inline virtual unsigned getRecvTimeout() const {
        return recvTimeout;
    } 

    inline virtual int getfd() const = 0;

    enum State {
        DISCONNECTED,
        READY,
        CONNECTED,
        ERROR
    };

    /** Returns current state of the connection */
    virtual const State& get_state() {return state;}

protected:
    State state = State::DISCONNECTED;
    const ConnectionType type;

    uint16_t sourcePort;

    // True means blocking, false means non-blocking
    bool blockmode;
    // True means reuse flag set, false means reuse flag not set
    bool reusemode;
    // Number of microseconds before a timeout on sends, 0 means no timeout 
    unsigned sendTimeout;
    // Number of microseconds before a timeout on receives, 0 means no timeout
    unsigned recvTimeout;
};

class ClientConnection : public Connection {
public:
    explicit ClientConnection(ConnectionType type, std::string address, uint16_t sourcePort, uint16_t destinationPort, bool blockmode, bool reusemode, unsigned sendTimeout, unsigned recvTimeout) : Connection(type, sourcePort, blockmode, reusemode, sendTimeout, recvTimeout), address(std::move(address)), destinationPort(destinationPort) {}
    ~ClientConnection() = default;

    class Factory;

    inline virtual const std::string& getAddress() const {
        return address;
    }

    inline virtual uint16_t getDestinationPort() const {
        return destinationPort;
    }

    virtual bool doConnect() = 0;
    virtual bool sendmsg(const uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool sendmsg(const uint8_t* const msg, unsigned length) const { return sendmsg(msg, length, 0); }

    virtual bool recvmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool recvmsg(uint8_t* const msg, unsigned length) const { return recvmsg(msg, length, MSG_WAITALL); }

    virtual bool peekmsg(uint8_t* const msg, unsigned length, int flags) const = 0;
    inline bool peekmsg(uint8_t* const msg, unsigned length) const { return peekmsg(msg, length, MSG_WAITALL); }

    virtual bool discardmsg(unsigned length) const = 0;
protected:
    std::string address;
    uint16_t destinationPort;

};

class HostConnection : public Connection {
public:
    explicit HostConnection(ConnectionType type, uint16_t hostPort, bool blockmode, bool reusemode, unsigned sendTimeout, unsigned recvTimeout, unsigned backlogSize) : Connection(type, hostPort, blockmode, reusemode, sendTimeout, recvTimeout), backlogSize(backlogSize) {};
    ~HostConnection() = default;

    class Factory;

    virtual std::unique_ptr<ClientConnection> acceptConnection() = 0;
protected:
    // Amount of connections to keep in the 'backlog' if we are contacted while busy with another connection.
    // Connections made with a full backlog are refused.
    unsigned backlogSize;
};


inline std::ostream& operator<<(std::ostream& stream, const ClientConnection& connection) {
    connection.print(stream);
    return stream;
}


class ClientConnection::Factory {
public:
    explicit Factory(ConnectionType type) : type(type) {}

    Factory& withAddress(std::string addr) {
        address = std::move(addr);
        return *this;
    }

    /**
     * Sets our local port to use for the connection.
     *
     * '''Convention:''' If `port=0` given (default), we use a random source port
     */
    Factory& withSourcePort(uint16_t port) {
        sourcePort = port;
        return *this;
    }

    Factory& withDestinationPort(uint16_t port) {
        destinationPort = port;
        return *this;
    }

    /** 
     * Sets blockmode. If `true` (default), socket calls like `send, recv, listen` etc block.
     * If `false`, these calls do not block. 
     * If there is nothing to do, the socket calls commonly return `-1` and set errno to `EWOULDBLOCK`.
     */
    Factory& withBlocking(bool blockmode) {
        this->blockmode = blockmode;
        return *this;
    }

    /**
     * Sets reusemode. If `true` (default), sockets are allowed to bind to previously used addresses.
     * If `false`, OS-dependent behaviour. Some OS's provide new addresses until reuse is the only option.
     */
    Factory& withReuse(bool reusemode) {
        this->reusemode = reusemode;
        return *this;
    }

    /**
     * Sets timeout. If 0 (default), there is no timeout.
     * If > 0, a timeout is set
     * If a timeout is reached and no data has been transferred, 
     * then socket calls commonly return `-1` and set errno to `EAGAIN` or `EWOULDBLOCK`
     */
    Factory& withSendTimeout(unsigned sendTimeout) {
        this->sendTimeout = sendTimeout;
        return *this;
    }

    /**
     * Sets timeout. If 0 (default), there is no timeout.
     * If > 0, a timeout is set
     * If a timeout is reached and no data has been transferred, 
     * then socket calls commonly return `-1` and set errno to `EAGAIN` or `EWOULDBLOCK`
     */
    Factory& withRecvTimeout(unsigned recvTimeout) {
        this->recvTimeout = recvTimeout;
        return *this;
    }

    virtual std::unique_ptr<ClientConnection> create() const = 0;

protected:
    ConnectionType type;
    std::string address;
    uint16_t sourcePort = 0, destinationPort = 0;
    bool blockmode = true;
    bool reusemode = true;
    unsigned sendTimeout = 0;
    unsigned recvTimeout = 0;
};

class HostConnection::Factory {
public:
    explicit Factory(ConnectionType type) : type(type) {}

    /**
    * Sets our local port to use for listening for connections.
    *
    * '''Convention:''' If `port=0` given (default), we use a random source port
    */
    Factory& withSourcePort(uint16_t port) {
        sourcePort = port;
        return *this;
    }


    /** 
     * Sets blockmode. If `true` (default), socket calls like `send, recv, listen` etc block.
     * If `false`, these calls do not block. 
     * If there is nothing to do, the socket calls commonly return `-1` and set errno to `EWOULDBLOCK`.
     *
     * '''WARNING:''' If non-blocking, all client communication will inherit the non-blocking property
     */
    Factory& withBlocking(bool blockmode) {
        this->blockmode = blockmode;
        return *this;
    }

    /**
     * Sets reusmode. If `true` (default), sockets are allowed to bind to previously used addresses.
     * If `false`, OS-dependent behaviour. Some OS's provide new addresses until reuse is the only option.
     */
    Factory& withReuse(bool reusemode) {
        this->reusemode = reusemode;
        return *this;
    }

    /**
     * Sets timeout. If 0 (default), there is no timeout.
     * If > 0, a timeout is set
     * If a timeout is reached and no data has been transferred, 
     * then socket calls commonly return `-1` and set errno to `EAGAIN` or `EWOULDBLOCK`
     */
    Factory& withSendTimeout(unsigned sendTimeout) {
        this->sendTimeout = sendTimeout;
        return *this;
    }

    /**
     * Sets timeout. If 0 (default), there is no timeout.
     * If > 0, a timeout is set
     * If a timeout is reached and no data has been transferred, 
     * then socket calls commonly return `-1` and set errno to `EAGAIN` or `EWOULDBLOCK`
     */
    Factory& withRecvTimeout(unsigned recvTimeout) {
        this->recvTimeout = recvTimeout;
        return *this;
    }

    /**
     * Sets backlog size. The backlog contains queued connections, which wait while we are busy.
     * If the backlog is full, new connections will be refused.
     */
    Factory& withBacklogSize(unsigned backlogSize) {
        this->backlogSize = backlogSize;
        return *this;
    }

    virtual std::unique_ptr<HostConnection> create() const = 0;

protected:
    ConnectionType type;
    uint16_t sourcePort = 0;
    bool blockmode = true;
    bool reusemode = true;
    unsigned sendTimeout = 0;
    unsigned recvTimeout = 0;
    unsigned backlogSize = 16;
};
#endif