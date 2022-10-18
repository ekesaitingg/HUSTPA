#include "common.h"
#include <amdev.h>

size_t serial_write(const void *buf, size_t offset, size_t len)
{
  for (size_t i = 0; i < len; ++i)
    _putc(((char *)buf)[i]);
  return len;
}

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
    [_KEY_NONE] = "NONE",
    _KEYS(NAME)};

size_t events_read(void *buf, size_t offset, size_t len)
{
  int keycode = read_key();
  if (keycode != _KEY_NONE)
  {
    if (keycode & 0x8000)
    {
      keycode ^= 0x8000;
      len = sprintf(buf, "kd %s\n", keyname[keycode]);
    }
    else if (!((keycode & ~0x8000) == _KEY_NONE))
    {
      len = sprintf(buf, "ku %s\n", keyname[keycode]);
    }
  }
  else
  {
    len = sprintf(buf, "t %u\n", uptime());
  }
  return len;
}

static char dispinfo[128] __attribute__((used)) = {};

size_t dispinfo_read(void *buf, size_t offset, size_t len)
{
  len = sprintf(buf, dispinfo + offset);
  return len;
}

size_t fb_write(const void *buf, size_t offset, size_t len)
{
  int x = (offset / 4) % screen_width();
  int y = (offset / 4) / screen_width();
  draw_rect((void *)buf, x, y, len / 4, 1);
  return len;
}

size_t fbsync_write(const void *buf, size_t offset, size_t len)
{
  return 0;
}

void init_device()
{
  Log("Initializing devices...");
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", screen_width(), screen_height());
}
