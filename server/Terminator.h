#pragma once

class ITerminator
{
public:
    virtual bool is_terminated() = 0;
    virtual void terminate() = 0;

    virtual ~ITerminator() {};
};

// simplest terminator implementation
class Terminator : public ITerminator
{
    volatile bool m_terminated = false;

public:
    bool is_terminated() override {
        return m_terminated;
    }
    void terminate() { m_terminated = true; }
};
