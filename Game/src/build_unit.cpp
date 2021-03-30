#include "os.h"

using namespace sv;

SV_USER Result user_initialize()
{
    SV_LOG_INFO("Init from Game\n");
    return Result_Success;
}

SV_USER void user_update()
{
    SV_LOG_INFO("Update from Game\n");
}

SV_USER Result user_close()
{
    SV_LOG_INFO("Close from Game\n");
    return Result_Success;
}
