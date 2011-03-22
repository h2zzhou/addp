#include "types.h"
#include "packet.h"

namespace addp {

static const mac_address MAC_ADDR_BROADCAST = {{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }};

class discovery_request : public packet
{
public:
    discovery_request(const mac_address mac_addr = MAC_ADDR_BROADCAST);
};

class discovery_response : public packet
{
public:
    discovery_response();
};

class static_net_config_request : public packet
{
public:
    static_net_config_request();
};

class static_net_config_response : public packet
{
public:
    static_net_config_response();
};

class reboot_request : public packet
{
public:
    reboot_request();
};

class reboot_response : public packet
{
public:
    reboot_response();
};

class dhcp_net_config_request : public packet
{
public:
    dhcp_net_config_request();
};

class dhcp_net_config_response : public packet
{
public:
    dhcp_net_config_response();
};

} // namespace addp
