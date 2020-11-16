#ifndef STREAMABLE_H
#define STREAMABLE_H

class Streamable {
public:
    virtual void write_stream(std::ostream& os) const = 0;
    virtual void read_stream(std::istream& is) = 0;
};

#endif
