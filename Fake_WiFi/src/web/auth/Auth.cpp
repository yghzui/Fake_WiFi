#include "Auth.h"

#include "../../common/AppState.h"

bool ensureAuth() {
  if (server.authenticate(AUTH_USER, AUTH_PASS)) return true;
  server.requestAuthentication(BASIC_AUTH, "Fake WiFi Admin", "请输入账号密码");
  return false;
}
