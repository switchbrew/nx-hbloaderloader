u8 Aarch64_Nop[] = {
  0x1f, 0x20, 0x03, 0xd5
};

enum {
  Exfat_RequiredOff=1,
  Exfat_RequiredOn=2,
  Exfat_DontCare=3
};

enum {
  TitleId_Fs = 0x0100000000000000
};

#define MakeVersion(major, minor, micro) \
  ((major << 16) | ((minor) << 8) | (micro))

#define MakePatch(title_id, min_major, min_minor, min_micro, max_major, max_minor, max_micro, exfat, offset, patch) \
  { (title_id), MakeVersion((min_major), (min_minor), (min_micro)), MakeVersion((max_major), (max_minor), (max_micro)), (exfat), (offset), patch, sizeof(patch) }

#define X 255

struct {
  u64 title_id;
  u32 min_version;
  u32 max_version;
  u32 exfat_requirement;
  u32 offset;
  const u8* patch;
  u32 patch_size;
} g_Patches[] =
{
  // == Patch to reenable nspwn. ==
  // Look for DF1E0071 A1F3FF54 in the decompressed FS kip1 binary.
  // Subtract 0x100 to remove the KIP1 header.
  // Add 4 to get the branch instruction (that will be NOP'd).

  MakePatch(TitleId_Fs, 5,0,0, 5,0,X, Exfat_RequiredOff, 0x248DC, Aarch64_Nop),
  MakePatch(TitleId_Fs, 5,0,0, 5,0,X, Exfat_RequiredOn,  0x248DC, Aarch64_Nop),
  MakePatch(TitleId_Fs, 5,1,0, 5,X,X, Exfat_RequiredOff, 0x2490C, Aarch64_Nop),
  MakePatch(TitleId_Fs, 5,1,0, 5,X,X, Exfat_RequiredOn,  0x2490C, Aarch64_Nop),
  MakePatch(TitleId_Fs, 6,0,0, 6,1,X, Exfat_RequiredOff, 0x74E30, Aarch64_Nop),
  MakePatch(TitleId_Fs, 6,0,0, 6,1,X, Exfat_RequiredOn,  0x80530, Aarch64_Nop),
  MakePatch(TitleId_Fs, 6,2,0, 6,2,X, Exfat_RequiredOff, 0x74E30, Aarch64_Nop),
  MakePatch(TitleId_Fs, 6,2,0, 6,2,X, Exfat_RequiredOn,  0x80530, Aarch64_Nop)
};

#undef X
