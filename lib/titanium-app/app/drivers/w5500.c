
#include "w5500.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_eth_driver.h"
#include "esp_mac.h"

/**
 * @brief Installs the GPIO ISR (Interrupt Service Routine) service.
 *
 * This function attempts to install the ISR service for GPIO interrupts with
 * default configuration (flags = 0). If the service is already installed,
 * indicated by the return code `ESP_ERR_INVALID_STATE`, the function treats
 * it as a successful outcome.
 *
 * @return KERNEL_SUCCESS if the service is successfully installed or already present.
 *         KERNEL_ERROR_FAIL_INSTALL_ISR on any other installation failure.
 */
static kernel_error_st install_isr_service(void) {
    const int GPIO_ISR_FLAGS_DEFAULT = 0;
    esp_err_t result                 = gpio_install_isr_service(GPIO_ISR_FLAGS_DEFAULT);

    if (result == ESP_OK) {
        return KERNEL_SUCCESS;
    }

    if (result == ESP_ERR_INVALID_STATE) {
        return KERNEL_SUCCESS;
    }

    return KERNEL_ERROR_FAIL_INSTALL_ISR;
}

/**
 * @brief Initializes the SPI bus for Ethernet communication.
 *
 * This function sets up the SPI bus for an Ethernet peripheral using the configuration
 * provided in `ethernet_spi_config`. It also installs the ISR service as a prerequisite.
 *
 * @note This function makes an exception by instantiating the SPI bus driver directly
 *       within the kernel. This is a special case, justified only for Ethernet SPI setup.
 *
 * @param ethernet_spi_config Pointer to the SPI configuration structure.
 *
 * @return KERNEL_SUCCESS on success.
 *         KERNEL_ERROR_NULL if the config pointer is NULL.
 *         KERNEL_ERROR_FAIL_INSTALL_ISR if ISR installation fails.
 *         KERNEL_ERROR_FAIL_SPI_BUS_INIT if SPI bus initialization fails.
 */
static kernel_error_st ethernet_spi_bus_init(ethernet_spi_config_st *ethernet_spi_config) {
    if (ethernet_spi_config == NULL) {
        return KERNEL_ERROR_NULL;
    }

    kernel_error_st err = install_isr_service();
    if (err != KERNEL_SUCCESS) {
        return err;
    }

    spi_bus_config_t buscfg = {
        .miso_io_num   = ethernet_spi_config->miso,
        .mosi_io_num   = ethernet_spi_config->mosi,
        .sclk_io_num   = ethernet_spi_config->sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    esp_err_t result = spi_bus_initialize(ethernet_spi_config->spi_host,
                                          &buscfg,
                                          SPI_DMA_CH_AUTO);
    if (result != ESP_OK) {
        return KERNEL_ERROR_FAIL_SPI_BUS_INIT;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Assign a locally administered MAC address to the SPI Ethernet module configuration.
 *
 * This function assigns a MAC address to the SPI Ethernet module configuration structure.
 * Since the W5500 (or similar SPI Ethernet modules) typically does not have a factory-programmed MAC address,
 * this function derives a unique locally administered MAC address using the ESP32's factory base MAC.
 *
 * It uses the `esp_efuse_mac_get_default()` API to retrieve the ESP32's base MAC address and
 * `esp_derive_local_mac()` to generate a new MAC address that is locally administered and safe
 * to use in local area networks (LANs).
 *
 * The resulting MAC address is stored in the `mac_addr` field of the `spi_eth_module_config_st` struct.
 *
 * @note Although the MAC is locally administered (not IEEE globally unique), this method is safe
 *       and appropriate for production use in environments where a global OUI is not required.
 *
 * @param[in,out] local_mac_address Pointer to a 6-byte array to receive the generated MAC address.
 *
 * @return KERNEL_SUCCESS on success.
 * @return KERNEL_ERROR_NULL if the input pointer is NULL.
 * @return KERNEL_ERROR_GETTING_DEFAULT_MAC if reading the base MAC from efuse fails.
 * @return KERNEL_ERROR_DERIVE_LOCAL_MAC if deriving the local MAC fails.
 */
kernel_error_st ethernet_assign_local_mac(uint8_t *local_mac_address) {
    if (local_mac_address == NULL) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t base_mac_addr[ETH_ADDR_LEN];

    esp_err_t result = esp_efuse_mac_get_default(base_mac_addr);
    if (result != ESP_OK) {
        return KERNEL_ERROR_GETTING_DEFAULT_MAC;
    }

    result = esp_derive_local_mac(local_mac_address, base_mac_addr);
    if (result != ESP_OK) {
        return KERNEL_ERROR_DERIVE_LOCAL_MAC;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Initialize an SPI-based W5500 Ethernet device.
 *
 * This function sets up the MAC, PHY, and SPI configuration for a W5500 Ethernet module.
 * It installs the Ethernet driver and assigns a custom MAC address.
 *
 * @param ethernet_device Pointer to the Ethernet device structure.
 *                        Must be initialized with hardware config and MAC address.
 *
 * @return KERNEL_SUCCESS on success.
 * @return KERNEL_ERROR_NULL if the pointer is NULL.
 * @return KERNEL_ERROR_ALLOC_ETH_MAC if MAC allocation fails.
 * @return KERNEL_ERROR_ALLOC_ETH_PHY if PHY allocation fails.
 * @return KERNEL_ERROR_INSTALL_ETH_DRIVER if driver installation fails.
 * @return KERNEL_ERROR_BURNING_MAC_ADDRESS if setting MAC address fails.
 */
static kernel_error_st ethernet_init_spi(ethernet_device_st *ethernet_device, esp_eth_handle_t *eth_handle) {
    if ((ethernet_device == NULL) || (eth_handle == NULL)){
        return KERNEL_ERROR_NULL;
    }

    eth_mac_config_t mac_config   = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_stack_size = ethernet_device->rx_stack_size;

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr         = ethernet_device->phy_addr;
    phy_config.reset_gpio_num   = ethernet_device->ethernet_hardware_config.phy_reset_gpio;

    spi_device_interface_config_t spi_devcfg = {
        .mode           = 0,
        .clock_speed_hz = ethernet_device->ethernet_hardware_config.ethernet_spi_config.spi_clock_mhz * 1000 * 1000,
        .queue_size     = 20,
        .spics_io_num   = ethernet_device->ethernet_hardware_config.ethernet_spi_config.cs};

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(
        ethernet_device->ethernet_hardware_config.ethernet_spi_config.spi_host,
        &spi_devcfg);
    w5500_config.int_gpio_num   = ethernet_device->ethernet_hardware_config.irq_gpio;
    w5500_config.poll_period_ms = ethernet_device->poll_period_ms;

    ethernet_device->mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    if (ethernet_device->mac == NULL) {
        return KERNEL_ERROR_ALLOC_ETH_MAC;
    }

    ethernet_device->phy = esp_eth_phy_new_w5500(&phy_config);
    if (ethernet_device->phy == NULL) {
        return KERNEL_ERROR_ALLOC_ETH_PHY;
    }

    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(ethernet_device->mac, ethernet_device->phy);
    esp_err_t result = esp_eth_driver_install(&eth_config_spi, eth_handle);
    if (result != ESP_OK) {
        return KERNEL_ERROR_INSTALL_ETH_DRIVER;
    }

    result = esp_eth_ioctl(*eth_handle, ETH_CMD_S_MAC_ADDR, ethernet_device->mac_addr);
    if (result != ESP_OK) {
        return KERNEL_ERROR_BURNING_MAC_ADDRESS;
    }

    return KERNEL_SUCCESS;
}

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
 *         - KERNEL_SUCCESS: Initialization successful.
 *         - KERNEL_ERROR_NULL: Null pointer passed as parameter.
 *         - KERNEL_ERROR_SPI_INIT: Failed to initialize SPI bus.
 *         - KERNEL_ERROR_ASSIGN_MAC: Failed to assign MAC address.
 *         - KERNEL_ERROR_ALLOC_ETH_MAC / _PHY: MAC/PHY driver allocation failed.
 *         - KERNEL_ERROR_INSTALL_ETH_DRIVER: Failed to install Ethernet driver.
 *         - KERNEL_ERROR_BURNING_MAC_ADDRESS: Failed to set MAC on driver.
 */
kernel_error_st w5500_initialize(ethernet_device_st *ethernet_device, esp_eth_handle_t *eth_handle) {
    if ((ethernet_device == NULL) || (eth_handle == NULL)){
        return KERNEL_ERROR_NULL;
    }
    
    kernel_error_st err = ethernet_spi_bus_init(&ethernet_device->ethernet_hardware_config.ethernet_spi_config);
    if (err != KERNEL_SUCCESS) {
        return err;
    }
    
    err = ethernet_assign_local_mac(ethernet_device->mac_addr);
    if (err != KERNEL_SUCCESS) {
        return err;
    }
    
    err = ethernet_init_spi(ethernet_device, eth_handle);
    if (err != KERNEL_SUCCESS) {
        return err;
    }

    return KERNEL_SUCCESS;
}
