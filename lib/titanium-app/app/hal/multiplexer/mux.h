#include <array>

#include "kernel/error/error_num.h"

#include "app/hal/drivers/tca9548a.h"
#include "app/hal/multiplexer/channel.h"

class Multiplexer {
   public:
    Multiplexer(TCA9548A* mux_driver)
        : mux_driver_(mux_driver) {
        for (uint8_t i = 0; i < channel_array.size(); ++i) {
            channel_array[i] = Channel(mux_driver, static_cast<TCA9548A::channel_index_e>(i));
        }
    }

    ~Multiplexer() = default;

    kernel_error_st disable() {
        if (this->mux_driver_ == nullptr) {
            return KERNEL_ERROR_NULL;
        }

        return this->mux_driver_->disable();
    }

    Channel& channel(size_t index) {
        return channel_array.at(index);  // bounds-checked
    }

    const Channel& channel(size_t index) const {
        return channel_array.at(index);
    }

    size_t channelCount() const {
        return channel_array.size();
    }

   private:
    std::array<Channel, static_cast<size_t>(TCA9548A::channel_index_e::NUM_OF_MUX_CHANNELS)> channel_array;
    TCA9548A* mux_driver_ = nullptr;
};
