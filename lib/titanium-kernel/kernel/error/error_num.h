/**
 * @file error_enum.h
 * @brief Defines standard error codes used throughout the kernel.
 *
 * This header file contains a set of error codes represented as an enum.
 * These codes provide a standardized way to handle and report errors
 * across different kernel modules.
 *
 * Each error is grouped into a hexadecimal range to improve traceability,
 * organization, and debugging in large systems.
 *
 * ----------------------------------------------------------------------------
 * Error Code Sections:
 * ----------------------------------------------------------------------------
 * 0x0000 - 0x00FF : General Errors
 * 0x0100 - 0x01FF : Queue, Task, and Thread Errors
 * 0x0200 - 0x02FF : MQTT-Related Errors
 * 0x0300 - 0x03FF : JSON, Schema, and Serialization Errors
 * 0x0400 - 0x04FF : Storage and NVS Errors
 * 0x0500 - 0x05FF : OTA Update Errors
 * 0x0600 - 0x06FF : Network and Wi-Fi Configuration Errors
 * 0x0700 - 0x07FF : System and Global Initialization Errors
 * 0x0800 - 0x08FF : Application and Command Errors
 *
 * ----------------------------------------------------------------------------
 * Usage Example:
 * ----------------------------------------------------------------------------
 * @code
 * kernel_error_t err = some_function();
 * if (err != KERNEL_ERROR_NONE) {
 *     printf("Error code: 0x%04X\n", err);
 * }
 * @endcode
 *
 * @note Ensure that functions returning these error codes follow
 *       the convention where `KERNEL_ERROR_NONE` (0x0000) indicates success.
 */

#ifndef ERROR_ENUM_H
#define ERROR_ENUM_H

/**
 * @enum kernel_error_st
 * @brief Standard error codes for the kernel.
 */
typedef enum kernel_error_s {
    /* -------- General (0x000) -------- */
    KERNEL_ERROR_NONE              = 0x0000,
    KERNEL_ERROR_FAIL              = 0x0001,
    KERNEL_ERROR_NULL              = 0x0002,
    KERNEL_ERROR_INVALID_ARG       = 0x0003,
    KERNEL_ERROR_INVALID_SIZE      = 0x0004,
    KERNEL_ERROR_INVALID_INDEX     = 0x0005,
    KERNEL_ERROR_NOT_FOUND         = 0x0006,
    KERNEL_ERROR_UNSUPPORTED_TYPE  = 0x0007,
    KERNEL_ERROR_FORMATTING        = 0x0008,
    KERNEL_ERROR_UNKNOWN_MAC       = 0x0009,
    KERNEL_ERROR_FUNC_POINTER_NULL = 0x000A,
    KERNEL_ERROR_TIMEOUT           = 0x000B,

    /* -------- Task/Queue (0x100) -------- */
    KERNEL_ERROR_TASK_CREATE     = 0x0100,
    KERNEL_ERROR_TASK_INIT       = 0x0101,
    KERNEL_ERROR_TASK_FULL       = 0x0102,
    KERNEL_ERROR_QUEUE_NULL      = 0x0103,
    KERNEL_ERROR_QUEUE_FULL      = 0x0104,
    KERNEL_ERROR_QUEUE_SEND      = 0x0105,
    KERNEL_ERROR_EMPTY_QUEUE     = 0x0106,
    KERNEL_ERROR_NO_MEM          = 0x0107,
    KERNEL_ERROR_MUTEX_INIT_FAIL = 0x0108,

    /* -------- MQTT (0x200) -------- */
    KERNEL_ERROR_MQTT_PUBLISH                = 0x0200,
    KERNEL_ERROR_MQTT_SUBSCRIBE              = 0x0201,
    KERNEL_ERROR_MQTT_REGISTER_FAIL          = 0x0202,
    KERNEL_ERROR_MQTT_URI_FAIL               = 0x0203,
    KERNEL_ERROR_MQTT_CONFIG_FAIL            = 0x0204,
    KERNEL_ERROR_MQTT_QUEUE_NULL             = 0x0205,
    KERNEL_ERROR_MQTT_ENQUEUE_FAIL           = 0x0206,
    KERNEL_ERROR_EMPTY_MQTT_TOPIC            = 0x0207,
    KERNEL_ERROR_MQTT_INVALID_TOPIC          = 0x0208,
    KERNEL_ERROR_MQTT_INVALID_QOS            = 0x0209,
    KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION = 0x020A,
    KERNEL_ERROR_MQTT_TOO_MANY_TOPICS        = 0x020B,

    /* -------- JSON/Serialization (0x300) -------- */
    KERNEL_ERROR_SERIALIZE_JSON   = 0x0300,
    KERNEL_ERROR_DESERIALIZE_JSON = 0x0301,
    KERNEL_ERROR_MISSING_FIELD    = 0x0302,
    KERNEL_ERROR_INVALID_TYPE     = 0x0303,
    KERNEL_ERROR_INVALID_COMMAND  = 0x0304,

    /* -------- Storage/NVS (0x400) -------- */
    KERNEL_ERROR_NVS_INIT            = 0x0400,
    KERNEL_ERROR_NVS_OPEN            = 0x0401,
    KERNEL_ERROR_NVS_SAVE            = 0x0402,
    KERNEL_ERROR_NVS_LOAD            = 0x0403,
    KERNEL_ERROR_NVS_ERASE_KEY       = 0x0404,
    KERNEL_ERROR_NVS_ERASE_ALL       = 0x0405,
    KERNEL_ERROR_NVS_NOT_INITIALIZED = 0x0406,

    /* -------- OTA (0x500) -------- */
    KERNEL_ERROR_NO_OTA_PARTITION_FOUND = 0x0500,
    KERNEL_ERROR_OTA_BEGIN_FAILED       = 0x0501,

    /* -------- Wi-Fi / Network (0x600) -------- */
    KERNEL_ERROR_EMPTY_SSID               = 0x0600,
    KERNEL_ERROR_STA_SSID_TOO_LONG        = 0x0601,
    KERNEL_ERROR_EMPTY_PASSWORD           = 0x0602,
    KERNEL_ERROR_STA_PASSWORD_TOO_LONG    = 0x0603,
    KERNEL_ERROR_READING_SSID             = 0x0604,
    KERNEL_ERROR_READING_PASSWORD         = 0x0605,
    KERNEL_ERROR_SOCK_CREATE_FAIL         = 0x0606,
    KERNEL_ERROR_RECEIVING_FORM_DATA      = 0x0607,
    KERNEL_ERROR_TIMESTAMP_FORMAT         = 0x0608,
    KERNEL_ERROR_WIFI_INIT                = 0x0608,
    KERNEL_ERROR_WIFI_SET_STORAGE         = 0x0609,
    KERNEL_ERROR_WIFI_SET_MODE            = 0x060A,
    KERNEL_ERROR_WIFI_START               = 0x060B,
    KERNEL_ERROR_AP_SET_IP_INFO           = 0x060C,
    KERNEL_ERROR_AP_DHCPS_START           = 0x060D,
    KERNEL_ERROR_AP_SET_CONFIG            = 0x060E,
    KERNEL_ERROR_AP_SET_BANDWIDTH         = 0x060F,
    KERNEL_ERROR_AP_SET_PS                = 0x0610,
    KERNEL_ERROR_AP_SSID_TOO_LONG         = 0x0611,
    KERNEL_ERROR_AP_PASSWORD_TOO_LONG     = 0x0612,
    KERNEL_ERROR_WIFI_NETIF_CREATE        = 0x0613,
    KERNEL_ERROR_STA_CONFIG               = 0x0614,
    KERNEL_ERROR_STA_CREDENTIALS          = 0x0615,
    KERNEL_ERROR_ETH_NET_INTERFACE_ALLOC  = 0x0616,
    KERNEL_ERROR_ETH_NET_INTERFACE_ATTACH = 0x0617,
    KERNEL_ERROR_DHCP_START               = 0x0618,
    KERNEL_ERROR_WIFI_EVENT_REGISTER      = 0x0619,
    KERNEL_ERROR_IP_EVENT_REGISTER        = 0x061A,
    KERNEL_ERROR_ETH_EVENT_REGISTER       = 0x061B,
    KERNEL_ERROR_ETHERNET_START           = 0x061C,

    /* -------- System Init (0x700) -------- */
    KERNEL_ERROR_INITIALIZATION_FAIL = 0x0700,
    KERNEL_ERROR_GLOBAL_EVENTS_INIT  = 0x0701,
    KERNEL_ERROR_GLOBAL_QUEUES_INIT  = 0x0702,

    /* -------- App/Commands (0x800) -------- */
    KERNEL_ERROR_INVALID_INTERFACE = 0x0800,

    /* -------- App/Sensors (0x900) -------- */
    KERNEL_ERROR_MUX_DISABLECHANNEL_ERROR  = 0x0900,
    KERNEL_ERROR_MUX_CHANNEL_ERROR         = 0x0901,
    KERNEL_ERROR_ADC_UPDATE_ERROR          = 0x0902,
    KERNEL_ERROR_ADC_CONFIGURE_ERROR       = 0x0903,
    KERNEL_ERROR_ADC_CONVERSION_ERROR      = 0x0904,
    KERNEL_ERROR_ADC_CONFIG_MISMATCH_ERROR = 0x0905,
    KERNEL_ERROR_ADC_READ_ERROR            = 0x0906,
    KERNEL_ERROR_FAILED_TO_ENCODE_PACKET   = 0x0907,
    KERNEL_ERROR_FAILED_TO_DECODE_PACKET   = 0x0908,

    /* -------- Drivers (0x1000) -------- */
    KERNEL_ERROR_FAIL_INSTALL_ISR    = 0x1000,
    KERNEL_ERROR_FAIL_SPI_BUS_INIT   = 0x1001,
    KERNEL_ERROR_GETTING_DEFAULT_MAC = 0x1002,
    KERNEL_ERROR_DERIVE_LOCAL_MAC    = 0x1003,
    KERNEL_ERROR_INSTALL_ETH_DRIVER  = 0x1004,
    KERNEL_ERROR_ALLOC_ETH_MAC       = 0x1005,
    KERNEL_ERROR_ALLOC_ETH_PHY       = 0x1006,
    KERNEL_ERROR_BURNING_MAC_ADDRESS = 0x1007,
    KERNEL_ERROR_MUX_RESET_ERROR     = 0x1008,
    KERNEL_ERROR_MUX_INIT_ERROR      = 0x1009,
    KERNEL_ERROR_ADC_INIT_ERROR      = 0x100A,

    /* -------- Hardware (0x1000) ------- */
    KERNEL_ERROR_GPIO_CONFIG_FAIL    = 0x1100,
    KERNEL_ERROR_GPIO_SET_LEVEL_FAIL = 0x1101,

} kernel_error_st;

#endif /* ERROR_ENUM_H */
