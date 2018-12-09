#include <switch.h>
#include "patches.h"

u32 __nx_applet_type = AppletType_None;

static char g_heap[0x10000];

void __libnx_initheap(void)
{
  extern char* fake_heap_start;
  extern char* fake_heap_end;

  fake_heap_start = &g_heap[0];
  fake_heap_end   = &g_heap[sizeof g_heap];
}

u32 g_Version;
bool g_IsFsExfat;

static Result _GetVersion()
{
  Result rc = fsInitialize();

  if (R_SUCCEEDED(rc))
  {
    rc = fsIsExFatSupported(&g_IsFsExfat);
    fsExit();

    if (R_SUCCEEDED(rc))
    {
      rc = setsysInitialize();

      if (R_SUCCEEDED(rc))
      {
        SetSysFirmwareVersion version;

        rc = setsysGetFirmwareVersion(&version);

        if (R_SUCCEEDED(rc))
          g_Version = MakeVersion(version.major, version.minor, version.micro);
      }
    }
  }

  return rc;
}

static Result _GetCodeRegion(Handle debug, u64* base_out, u64* size_out)
{
  Result rc;
  MemoryInfo info;
  u32 crap;

  info.addr = 0;
  info.size = 0;

  while (R_SUCCEEDED(rc = svcQueryDebugProcessMemory(&info, &crap, debug, info.addr + info.size)))
  {
    if (info.type == MemType_CodeStatic)
    {
      *base_out = info.addr;
      *size_out = info.size;
      return 0;
    }
  }

  return -1;
}

static Result _GetTitleId(Handle debug, u64* title_id_out)
{
  union {
    u8 raw[0x40];
    struct {
      u32 type;
      u32 flags;
      u64 thread_id;
      u64 title_id; // Only if type == 0
    };
  } event;

  Result rc;

  while (R_SUCCEEDED(rc = svcGetDebugEvent(event.raw, debug)))
  {
    if (event.type == 0)
    {
      *title_id_out = event.title_id;
      return 0;
    }
  }

  return rc;
}

static Result _PatchByTitleId(Handle debug, u64 title_id)
{
  u64 base, size;
  Result rc = _GetCodeRegion(debug, &base, &size);

  if (R_SUCCEEDED(rc))
  {
    size_t i;
    for (i=0; i<sizeof(g_Patches)/sizeof(g_Patches[0]); i++)
    {
      if (g_Patches[i].title_id != title_id)
        continue;
      if (g_Patches[i].min_version > g_Version)
        continue;
      if (g_Patches[i].max_version < g_Version)
        continue;

      switch (g_Patches[i].exfat_requirement)
      {
        case Exfat_RequiredOff:
          if (g_IsFsExfat) continue;
          break;

        case Exfat_RequiredOn:
          if (!g_IsFsExfat) continue;
          break;
      }

      rc = svcWriteDebugProcessMemory(
        debug, g_Patches[i].patch, base + g_Patches[i].offset, g_Patches[i].patch_size);

      if (R_FAILED(rc))
        break;
    }
  }

  return rc;
}

static Result _PatchByPid(u64 pid)
{
  Handle debug;
  Result rc = svcDebugActiveProcess(&debug, pid);

  if (R_SUCCEEDED(rc))
  {
    u64 title_id;
    rc = _GetTitleId(debug, &title_id);

    if (R_SUCCEEDED(rc))
      rc = _PatchByTitleId(debug, title_id);

    svcCloseHandle(debug);
  }

  return rc;
}

static Result _PatchBuiltins()
{
  u64 own_pid;
  Result rc = svcGetProcessId(&own_pid, CUR_PROCESS_HANDLE);

  if (R_SUCCEEDED(rc))
  {
    size_t pid;
    for (pid=0; pid<80; pid++)
    {
      if (pid != own_pid)
        _PatchByPid(pid);
    }
  }

  return rc;
}

static Result _RegisterHbl()
{
  Result rc = lrInitialize();

  if (R_SUCCEEDED(rc))
  {
    LrLocationResolver resolver;
    rc = lrOpenLocationResolver(FsStorageId_NandSystem, &resolver);

    if (R_SUCCEEDED(rc))
    {
      rc = lrLrRedirectProgramPath(&resolver, 0x010000000000100D, "@Sdcard:/bootloader/hbl.nsp");
      serviceClose(&resolver.s);
    }

    lrExit();
  }

  return rc;
}

int main(int argc, char* argv[])
{
  Result rc;

  rc = _GetVersion();
  if (R_FAILED(rc)) fatalSimple(rc);

  rc = _PatchBuiltins();
  if (R_FAILED(rc)) fatalSimple(rc);

  rc = _RegisterHbl();
  if (R_FAILED(rc)) fatalSimple(rc);

  return 0;
}
