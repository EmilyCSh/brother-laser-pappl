import brother_laser.common;
import brother_laser.pappl;
import brother_laser.printer;

//NOLINTBEGIN
#include <pappl/pappl.h>
static pappl_pr_driver_t drivers[] = {
    {"test_pappl", "Test_PAPPL_printer", NULL, NULL},
    {"brother_hl1110", "Brother HL-1110 series", NULL, NULL},
};

static bool driver_callback(
    [[maybe_unused]] pappl_system_t* system,
    [[maybe_unused]] const char* driver_name,
    [[maybe_unused]] const char* device_uri,
    [[maybe_unused]] const char* device_id,
    [[maybe_unused]] pappl_pr_driver_data_t* driver_data,
    [[maybe_unused]] ipp_t** driver_attrs,
    [[maybe_unused]] void* data)
{
    return true;
}

auto main(int argc, char* argv[]) -> int
{
    return (papplMainloop(
        argc,
        argv,
        "0.1",
        "Brother monochrome laser printer application",
        (int)(sizeof(drivers) / sizeof(drivers[0])),
        drivers,
        NULL,
        driver_callback,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL));
    //NOLINTEND
}