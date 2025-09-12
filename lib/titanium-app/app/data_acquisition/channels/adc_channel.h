#include "kernel/error/error_num.h"

#include "app/drivers/ads1115.h"

/**
 * @brief Represents a single analog input channel using an ADS1115 driver.
 */
class AnalogReadChannel {
public:
    /**
     * @brief Construct an AnalogReadChannel.
     * @param driver Pointer to an initialized ADS1115 driver (must remain valid for lifetime of this object)
     */
    explicit AnalogReadChannel(Ads1115Driver* driver)
        : analog_read_driver_(driver) {}

    ~AnalogReadChannel() = default;

    /**
     * @brief Read the current analog value from the channel.
     * @return Kernel error status.
     */
    kernel_error_st read();

private:
    Ads1115Driver* analog_read_driver_;
};
