#include "ethernet_manager.h"

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "kernel/logger/logger.h"

/* Network Bridge Global Variables */
static const char* TAG             = "Network Bridge";  ///< Log tag for network bridge
static bool is_ethernet_ip_set     = false;             ///< Ethernet connection status flag
static esp_netif_t* eth_netif      = NULL;              ///< Pointer to the Ethernet network interface instance.
static esp_eth_handle_t eth_handle = NULL;              ///< Handle for the Ethernet driver instance; currently supports only one external Ethernet device.

/**
 * @brief Static initialization structure for the network bridge configuration.
 *
 * Contains the Ethernet device hardware configuration including SPI pins,
 * interrupt and PHY reset pins, polling parameters, and MAC address storage.
 * It also holds a pointer to the associated network_bridge interface.
 */
static ethernet_device_st ethernet_device = {
    .ethernet_hardware_config = {
        .ethernet_spi_config = {
            .miso          = GPIO_NUM_19,  ///< SPI MISO pin number
            .mosi          = GPIO_NUM_23,  ///< SPI MOSI pin number
            .sclk          = GPIO_NUM_18,  ///< SPI Clock pin number
            .cs            = GPIO_NUM_5,   ///< SPI Chip Select pin number
            .spi_host      = SPI3_HOST,    ///< SPI host to use (SPI3)
            .spi_clock_mhz = 10,           ///< SPI clock frequency in MHz
        },
        .irq_gpio       = -1,           ///< IRQ GPIO pin (-1 if unused)
        .phy_reset_gpio = GPIO_NUM_26,  ///< PHY reset GPIO pin number
    },
    .poll_period_ms = 10,          ///< Polling period in milliseconds
    .phy_addr       = 1,           ///< PHY address on the MDIO bus
    .mac_addr       = {0},         ///< MAC address placeholder (initialized to zero)
    .rx_stack_size  = (2048 * 4),  ///< RX stack size in bytes
};

/**
 * @brief Handle Ethernet events and update connection status accordingly.
 *
 * Logs Ethernet events such as connection, disconnection, start, and stop.
 * Updates the internal connection status flag on disconnection.
 *
 * @param[in] event_id    The Ethernet event ID.
 * @param[in] event_data  Pointer to event-specific data (unused here).
 */
void ethernet_manager_handle_events(int32_t event_id, void* event_data) {
    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            logger_print(INFO, TAG, "Ethernet cable connected.");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            logger_print(WARN, TAG, "Ethernet cable disconnected.");
            is_ethernet_ip_set = false;
            break;
        case ETHERNET_EVENT_START:
            logger_print(INFO, TAG, "Ethernet interface started.");
            break;
        case ETHERNET_EVENT_STOP:
            logger_print(INFO, TAG, "Ethernet interface stopped.");
            break;
        default:
            logger_print(DEBUG, TAG, "Received unknown Ethernet event ID: %d", event_id);
            break;
    }
}

/**
 * @brief Handle the event when the Ethernet interface receives an IP address.
 *
 * This function is called when the Ethernet interface successfully obtains an
 * IP address via DHCP. It logs the assigned IP, netmask, and gateway information,
 * and sets the internal connection status flag to indicate that Ethernet is online.
 *
 * @param[in] event_data  Pointer to the IP event data, which should be cast
 *                        to `ip_event_got_ip_t*`.
 */
void ethernet_manager_sta_got_ip(void* event_data) {
    ip_event_got_ip_t* event           = (ip_event_got_ip_t*)event_data;
    const esp_netif_ip_info_t* ip_info = &event->ip_info;

    logger_print(DEBUG, TAG, "Ethernet Got IP Address");
    logger_print(DEBUG, TAG, "-----------------------");
    logger_print(DEBUG, TAG, "IP: " IPSTR, IP2STR(&ip_info->ip));
    logger_print(DEBUG, TAG, "MASK: " IPSTR, IP2STR(&ip_info->netmask));
    logger_print(DEBUG, TAG, "GW: " IPSTR, IP2STR(&ip_info->gw));
    logger_print(DEBUG, TAG, "-----------------------");

    is_ethernet_ip_set = true;
}

void ethernet_manager_lost_ip(void) {
    logger_print(WARN, TAG, "Ethernet lost IP. Restarting DHCP...");
    is_ethernet_ip_set = false;

    if (eth_netif) {
        esp_netif_dhcpc_stop(eth_netif);
        esp_netif_dhcpc_start(eth_netif);
    }
}

/**
 * @brief Retrieve the current Ethernet connection status.
 *
 * This function returns whether the Ethernet interface is currently connected
 * and has obtained a valid IP address.
 *
 * @return true if Ethernet is connected and has a valid IP; false otherwise.
 */
bool ethernet_manager_get_connection_status(void) {
    return is_ethernet_ip_set;
}

/**
 * @brief Initialize and start the Ethernet interface and driver.
 *
 * This function:
 * - Creates a new Ethernet network interface.
 * - Installs and initializes the Ethernet driver (e.g., W5500).
 * - Attaches the driver to the ESP-IDF network interface glue layer.
 * - Starts the DHCP client to acquire an IP address.
 * - Starts the Ethernet interface.
 *
 * After this function returns successfully, the Ethernet interface should
 * be ready to connect to the network and obtain an IP.
 *
 * @return KERNEL_SUCCESS if the Ethernet interface was successfully initialized
 *         and started; otherwise, returns a specific kernel error code.
 */
kernel_error_st ethernet_manager_initialize() {
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();

    eth_netif = esp_netif_new(&cfg);

    kernel_error_st err = w5500_initialize(&ethernet_device, &eth_handle);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "W5500 driver initialization failed: %d", err);
        return err;
    }

    esp_err_t result = esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to attach Ethernet netif: %s", esp_err_to_name(result));
        return KERNEL_ERROR_ETH_NET_INTERFACE_ATTACH;
    }

    result = esp_netif_dhcpc_start(eth_netif);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start DHCP client: %s", esp_err_to_name(result));
        return KERNEL_ERROR_DHCP_START;
    }

    logger_print(INFO, TAG, "DHCP client started successfully");

    result = esp_eth_start(eth_handle);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start Ethernet: %s", esp_err_to_name(result));
        return KERNEL_ERROR_ETHERNET_START;
    }

    return KERNEL_SUCCESS;
}
