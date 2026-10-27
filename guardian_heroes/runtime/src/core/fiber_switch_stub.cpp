#include <rex/thread/fiber.h>
#if REX_PLATFORM_SWITCH
namespace rex::thread {
thread_local Fiber* Fiber::tls_current_ = nullptr;
Fiber* Fiber::ConvertCurrentThread(){ return nullptr; }
Fiber* Fiber::Create(size_t,void(*)(void*),void*){ return nullptr; }
void Fiber::SwitchTo(Fiber*){}
void Fiber::Destroy(){ delete this; }
}
#endif
