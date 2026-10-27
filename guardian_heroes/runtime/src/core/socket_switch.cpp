#include <rex/net/socket.h>
#include <switch.h>
#include <unistd.h>

namespace rex::net {
int socket_close(SocketHandle handle) { return ::close(static_cast<int>(handle)); }
int socket_ioctl(SocketHandle handle, uint32_t cmd, uint8_t* arg) {
  (void)handle;
  (void)cmd;
  (void)arg;
  return -1;
}
}  // namespace rex::net
