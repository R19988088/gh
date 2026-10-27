#include <rex/thread/atomic.h>
namespace rex::thread {
int32_t atomic_inc(volatile int32_t* p){return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST);} int32_t atomic_dec(volatile int32_t* p){return __atomic_sub_fetch(p,1,__ATOMIC_SEQ_CST);}
int32_t atomic_exchange(int32_t v,volatile int32_t*p){return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST);} int64_t atomic_exchange(int64_t v,volatile int64_t*p){return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST);}
int32_t atomic_exchange_add(int32_t v,volatile int32_t*p){return __atomic_fetch_add(p,v,__ATOMIC_SEQ_CST);} int64_t atomic_exchange_add(int64_t v,volatile int64_t*p){return __atomic_fetch_add(p,v,__ATOMIC_SEQ_CST);}
bool atomic_cas(int32_t a,int32_t b,volatile int32_t*p){return __atomic_compare_exchange_n(p,&a,b,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);} bool atomic_cas(int64_t a,int64_t b,volatile int64_t*p){return __atomic_compare_exchange_n(p,&a,b,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);}
void atomic_store_release(int32_t v,volatile int32_t*p){__atomic_store_n(p,v,__ATOMIC_RELEASE);}
}
