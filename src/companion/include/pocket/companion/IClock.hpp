#pragma once

#include <cstdint>
#include <QDateTime>

namespace Pocket::Companion {

class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t nowSecs() const = 0;
};

class SystemClock : public IClock {
public:
    int64_t nowSecs() const override {
        return QDateTime::currentSecsSinceEpoch();
    }
};

class TestClock : public IClock {
public:
    explicit TestClock(int64_t initialSecs = 1700000000) : m_nowSecs(initialSecs) {}

    int64_t nowSecs() const override {
        return m_nowSecs;
    }

    void setNowSecs(int64_t secs) {
        m_nowSecs = secs;
    }

    void advanceSeconds(int64_t seconds) {
        m_nowSecs += seconds;
    }

    void advanceHours(int hours) {
        m_nowSecs += static_cast<int64_t>(hours) * 3600;
    }

private:
    int64_t m_nowSecs;
};

} // namespace Pocket::Companion
