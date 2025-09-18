#include <iostream>
#include <array>

constexpr size_t MAX_MUXES    = 2;
constexpr size_t MAX_CHANNELS = 8;
constexpr size_t MAX_SENSORS  = 4;

// --- Sensor ---
class Sensor {
public:
    Sensor(int id = -1) : id(id), valid(false) {}
    void init(int newId) { id = newId; valid = true; }

    void update() {
        if (valid) {
            std::cout << "      Sensor " << id << " updated\n";
        }
    }

    bool isValid() const { return valid; }

private:
    int id;
    bool valid;
};

// --- Channel ---
class Channel {
public:
    Channel(int id = -1) : id(id), valid(false), sensorCount(0) {}
    void init(int newId) { id = newId; valid = true; }

    bool addSensor(int sensorId) {
        if (sensorCount < MAX_SENSORS) {
            sensors[sensorCount].init(sensorId);
            sensorCount++;
            return true;
        }
        return false;
    }

    void update() {
        if (valid) {
            std::cout << "   Channel " << id << " active\n";
        }
    }

    auto begin() { return sensors.begin(); }
    auto end()   { return sensors.begin() + sensorCount; }

private:
    int id;
    bool valid;
    size_t sensorCount;
    std::array<Sensor, MAX_SENSORS> sensors;
};

// --- Mux ---
class Mux {
public:
    Mux(int id = -1) : id(id), valid(false), channelCount(0) {}
    void init(int newId) { id = newId; valid = true; }

    bool addChannel(int channelId) {
        if (channelCount < MAX_CHANNELS) {
            channels[channelCount].init(channelId);
            channelCount++;
            return true;
        }
        return false;
    }

    void enable() {
        if (valid) {
            std::cout << "Mux " << id << " enabled\n";
        }
    }

    void disable() {
        if (valid) {
            std::cout << "Mux " << id << " disabled\n";
        }
    }

    auto begin() { return channels.begin(); }
    auto end()   { return channels.begin() + channelCount; }

private:
    int id;
    bool valid;
    size_t channelCount;
    std::array<Channel, MAX_CHANNELS> channels;
};

// --- Main ---
int main() {
    std::array<Mux, MAX_MUXES> muxes;

    muxes[0].init(1);
    muxes[1].init(2);

    muxes[0].addChannel(10);
    muxes[0].begin()->addSensor(101);
    muxes[0].begin()->addSensor(102);

    muxes[1].addChannel(20);
    muxes[1].begin()->addSensor(201);

    // --- Manual nested loop style ---
    for (auto& mux : muxes) {
        // mux.enable();  // mux-level things

        for (auto& channel : mux) {
            channel.enable();  // channel-level things

            for (auto& sensor : channel) {
                sensor.update();  // sensor-level things
            }
        }

        mux.disable();  // cleanup mux
    }

    return 0;
}
