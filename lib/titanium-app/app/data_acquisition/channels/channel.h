#include "kernel/error/error_num.h"

#include "app/drivers/tca9548a.h"

class Channel {
   public:
    explicit Channel(TCA9548A* mux_driver, TCA9548A::channel_index_e channel_index)
        : mux_driver_(mux_driver_) {}

    ~Channel() = default;

    kernel_error_st enable() {
        return this->mux_driver_->enable_channel(this->channel_index_);
    }

   private:
    TCA9548A* mux_driver_;
    TCA9548A::channel_index_e channel_index_;
};
