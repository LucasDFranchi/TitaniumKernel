#pragma once

#include "esp_eth.h"

#include "kernel/error/error_num.h"

/**
 * @brief SPI configuration parameters for an Ethernet device.
 */
typedef struct ethernet_spi_config_t {
    uint8_t miso;           ///< GPIO number for SPI MISO
    uint8_t mosi;           ///< GPIO number for SPI MOSI
    uint8_t sclk;           ///< GPIO number for SPI SCLK
    uint8_t cs;             ///< GPIO number for SPI chip select (CS)
    int8_t spi_host;        ///< SPI host (e.g., HSPI_HOST, VSPI_HOST)
    uint8_t spi_clock_mhz;  ///< SPI clock speed in MHz
} ethernet_spi_config_st;

/**
 * @brief Hardware configuration for an SPI-based Ethernet device.
 */
typedef struct ethernet_hardware_config_t {
    ethernet_spi_config_st ethernet_spi_config;  ///< SPI configuration
    int8_t irq_gpio;                             ///< Interrupt GPIO number from Ethernet chip
    int8_t phy_reset_gpio;                       ///< GPIO for PHY reset line
} ethernet_hardware_config_st;

/**
 * @brief Device initialization state.
 */
typedef enum dev_state_e {
    DEV_STATE_UNINITIALIZED,
    DEV_STATE_INITIALIZED,
} dev_state_et;

/**
 * @brief Runtime instance of a W5500 Ethernet device.
 *
 * This structure must be initialized by the user with hardware configuration
 * and PHY address before calling `w5500_initialize()`.
 * After successful initialization, MAC and PHY handles will be populated.
 */
typedef struct ethernet_device_t {
    ethernet_hardware_config_st ethernet_hardware_config;  ///< Physical hardware config
    const uint32_t poll_period_ms;                         ///< Poll period for interrupt handling (ms)
    const uint32_t rx_stack_size;                          ///< Stack size for RX task (unused for now)
    uint8_t phy_addr;                                      ///< PHY address on SPI bus
    uint8_t mac_addr[ETH_ADDR_LEN];                        ///< MAC address to be set on device
    esp_eth_mac_t *mac;                                    ///< Pointer to MAC instance
    esp_eth_phy_t *phy;                                    ///< Pointer to PHY instance
} ethernet_device_st;

/**
 * @brief Initializes the W5500 Ethernet interface over SPI.
 *
 * This function performs the full initialization sequence for a W5500-based
 * SPI Ethernet module. It:
 *   1. Initializes the SPI bus with the configured pins and host.
 *   2. Assigns a locally administered MAC address to the Ethernet device.
 *   3. Configures and installs the W5500 Ethernet driver with MAC/PHY settings.
 *
 * @param[in,out] ethernet_device Pointer to a configured ethernet_device_st structure.
 *                                The structure must be pre-initialized with hardware
 *                                configuration, poll period, and PHY address.
 *                                On success, the MAC and PHY handles are populated.
 *
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE: Initialization successful.
 *         - KERNEL_ERROR_NULL: Null pointer passed as parameter.
 *         - KERNEL_ERROR_SPI_INIT: Failed to initialize SPI bus.
 *         - KERNEL_ERROR_ASSIGN_MAC: Failed to assign MAC address.
 *         - KERNEL_ERROR_ALLOC_ETH_MAC / _PHY: MAC/PHY driver allocation failed.
 *         - KERNEL_ERROR_INSTALL_ETH_DRIVER: Failed to install Ethernet driver.
 *         - KERNEL_ERROR_BURNING_MAC_ADDRESS: Failed to set MAC on driver.
 */
kernel_error_st w5500_initialize(ethernet_device_st *ethernet_device, esp_eth_handle_t *esp_eth_handle);
