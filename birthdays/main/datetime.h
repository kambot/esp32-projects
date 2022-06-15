#pragma once

// ====================================================================
// INCLUDES 
// ====================================================================

#include "common.h"


// ====================================================================
// DEFINES 
// ====================================================================

#define DEFAULT_SNTP_SERVER_ADDR    "time.google.com"           /**< Default SNTP server address (time.google.com) */

#define TIME_FORMAT_IOT             "TIME_FORMAT_IOT"  /**< Time format used by the AWS IoT Server */
#define TIME_FORMAT_DEFAULT         "%Y-%m-%d %H:%M:%S"         /**< Default time format (%Y-%m-%d %H:%M:%S) */

#define TIME_STR_MAX_SIZE           64                          /**< Max formatted time string length */
#define TIME_SERVER_STR_MAX_SIZE    32                          /**< Max server address string length */

#define TIME_DEFAULT_SYNC_PERIOD        (5*60)                      /**< Default auto update period (5 minutes) */

#define TIME_EPOCH                       1970                        /**< Default \a base_year, the epoch is Unix time 0 (midnight 1/1/1970) */


#define SECONDS_IN_YEAR      31536000
#define SECONDS_IN_YEAR_LEAP 31622400
#define SECONDS_IN_DAY       86400
#define SECONDS_IN_HOUR      3600
#define SECONDS_IN_MINUTE    60

// ====================================================================
// MACROS
// ====================================================================

#define TIME_UPDATED_ONCE()    (tm_gmt.tm_year >= TIME_TM_TEST_YEAR)

#define TIME_NOW()      time_now(TIME_EPOCH) /**< Current timestamp */

// ====================================================================
// GLOBAL VARS
// ====================================================================

extern struct tm tm_local;
extern struct tm tm_gmt;

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================


///
/// @brief Initialize the SNTP functionality
///
/// @note This also enabled the auto update functionality, which can be disabled by calling `time_disable_auto_update()`
///
/// @return
///         - TRUE  Successfully initialized
///         - FALSE Failed to initialize
bool time_init(int sync_interval);

bool time_get_gmt_offset(float* offset);

bool time_set_timezone(char* timezone);

bool time_set_timezone_id(char* timezone_id);

///
/// @brief Set the SNTP server to update the time from
///
/// @note The default server `DEFAULT_SNTP_SERVER_ADDR` will be used if `time_init()` is called before this
/// @note It is possible to change the server address after initialization
///
/// @param server The address of the server
void time_set_sntp_server(char* server);

void time_set_sync_interval(int sync_interval);

int64_t time_update_get_last_try_time();

///
/// @brief Trigger a time update from the SNTP server
///
/// @note The time update may still succeed if this function times out
///
/// @param timeout_ms Number of milliseconds to wait for a successful time update before returning
bool time_update_time(int timeout_ms);

///
/// @brief The number of seconds since the time was successfully updated from the SNTP server
///
/// @return
///         - (-1) The time has never successfully been set by the server
int time_since_last_update();


///
/// @brief The time since boot, in microseconds, since the time was successfully updated from the SNTP server
///
/// @return
///         - (0) The time has never successfully been set by the server
int64_t time_last_update_time_us();

///
/// @brief Tells whether or not the last call to `time_update_time()` succeeded or not
///
/// @return
///         - TRUE  The time successfully updated from the server
///         - FALSE The time never updated
bool time_success();

///
/// @brief Stop communication from the SNTP server
///
void time_stop();


///
/// @brief Get the current time, in seconds
///
/// @param base_year Year to count the seconds from
///
/// @return The number of seconds since midnight on January 1, of `base_year`
time_t time_now(uint16_t base_year);

time_t time_now_local(uint16_t base_year);

///
/// @brief Get the current time as a formatted time string
///
/// @param format The format of the string
/// @param ret_str The resulting string will be retuned here
void time_now_str(const char* format, char* ret_str);

void time_now_str_local(const char* format, char* ret_str);

void time_print_now();

void time_print_now_local();

///
/// @brief Get the current time as a \a tm struct
///
/// @return Time structure with the current time broken down into its components
struct tm time_now_tm();

struct tm time_now_tm_local();


///
/// @brief Convert a timestamp to a formatted time string
///
/// @param timestamp The number of seconds since midnight on January 1, of `base_year`
/// @param base_year The reference year for timestamp
/// @param format The format of the string
/// @param ret_str The resulting string will be retuned here
void time_str_from_timestamp(long long timestamp, uint16_t base_year, const char* format, char* ret_str, bool is_gmt);

///
/// @brief Convert a \a tm struct to a formatted time string
///
/// @param time_info The time structure
/// @param format The format of the string
/// @param ret_str The resulting string will be returned here
void time_str_from_tm(struct tm time_info, const char* format, char* ret_str, bool is_gmt);



///
/// @brief Convert a timestamp to a \a tm struct
///
/// @param timestamp The number of seconds since midnight on January 1, of `base_year`
/// @param base_year The reference year for timestamp
///
/// @return Time structure with the timestamp broken down into its components
struct tm time_tm_from_timestamp(long long timestamp, uint16_t base_year);

///
/// @brief Convert a \a tm struct to a timestamp
///
/// @param time_info The time structure
/// @param base_year The reference year for the returned timestamp
///
/// @return The number of seconds since midnight on January 1, of `base_year` of the time contained in the time structure
time_t time_timestamp_from_tm(struct tm time_info, uint16_t base_year);


uint16_t get_day_of_year(uint8_t month, uint8_t day, uint16_t year);
int get_num_leap_years(int start_year, int end_year); // doesn't include end year in count
bool is_leap_year(int year);
int get_days_in_month(int month, int year);
int get_days_in_month2(int month);


// %a  Abbreviated weekday name  Sun
// %A  Full weekday name  Sunday
// %b  Abbreviated month name  Mar
// %B  Full month name  March
// %c  Date and time representation  Sun Aug 19 02:56:02 2012
// %d  Day of the month (01-31)  19
// %H  Hour in 24h format (00-23)  14
// %I  Hour in 12h format (01-12)  05
// %j  Day of the year (001-366)  231
// %m  Month as a decimal number (01-12)  08
// %M  Minute (00-59)  55
// %p  AM or PM designation  PM
// %S  Second (00-61)  02
// %U  Week number with the first Sunday as the first day of week one (00-53)  33
// %w  Weekday as a decimal number with Sunday as 0 (0-6)  4
// %W  Week number with the first Monday as the first day of week one (00-53)  34
// %x  Date representation  08/19/12
// %X  Time representation  02:50:06
// %y  Year, last two digits (00-99)  01
// %Y  Year  2012
// %Z  Timezone name or abbreviation  CDT
// %%  A % sign  %

