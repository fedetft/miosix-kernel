#include "../UUID/UUID.h"
namespace GPT 
{
#define MAKE_UUID_NAME_PAIR(uuid, name) {DEF_UUID(uuid), name}
constexpr std::initializer_list<std::pair<UUID::UUID, 
                const char*>> GPT_PARTITION_IDS = {
                    MAKE_UUID_NAME_PAIR("00000000-0000-0000-0000-000000000000", "Empty"),
                    MAKE_UUID_NAME_PAIR("024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme"),
                    MAKE_UUID_NAME_PAIR("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft Basic Data Partition"),
                };
} // namespace GPT