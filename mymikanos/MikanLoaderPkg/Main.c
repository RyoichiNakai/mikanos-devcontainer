#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

// EDK2の型定義
// https://edk2-docs.gitbook.io/edk-ii-dec-specification/3_edk_ii_dec_file_format/32_package_declaration_dec_definitions

struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  // メモリ領域が小さすぎて、メモリマップ全てを収めることができない場合
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  /**
   * gBS->GetMemoryMapの仕様
   *
   * EFI_STATUS GetMemoryMap(
   *  * IN: メモリマップ書き込み用のメモリ領域の大きさ
   *  * OUT: 実際のメモリマップの大きさ 
   *  IN OUT UNITN *MemoryMapSize,
   *  
   *  * メモリマップ書き込み用のメモリ領域の先頭ポインタを設定
   *  * IN: メモリ領域の先頭ポインタを入力する
   *  * OUT: メモリマップが書き込まれる
   *  IN OUT EFI_MEMOEY_DESCRIPTOR *MemoryMap,
   * 
   *  * メモリマップを識別するために値を書き込む変数を指定
   *  OUT UINTN *MapKey,
   * 
   *  * メモリマップの個々の行を表すメモリディスクリプタのバイト数
   *  OUT UINTN *DescriptorSize,
   * 
   *  * メモリディスクリプタの構造体のバージョン番号を表す
   *  OUT UINT32 *DescriptorVersion
   * )
   */
  return gBS->GetMemoryMap(
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version
  );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL *file) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (
    iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
    iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
    iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    len = AsciiSPrint(
      buf, 
      sizeof(buf),
      "%u, %x, %-ls, %08lx, %lx, %lx\n",
      i,
      desc->Type,
      GetMemoryTypeUnicode(desc->Type),
      desc->PhysicalStart,
      desc->NumberOfPages,
      desc->Attribute & 0xffffflu
    );
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE* system_table) {
  Print(L"Hello, Mikan World!\n");

  // メモリマップをCSVファイルに書き込み処理
  CHAR8 memmap_buf[4096 * 4]; // 16KiB
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL* memmap_file;
  // 書き込み先のファイルを開く関数
  root_dir->Open(
    root_dir, &memmap_file, L"\\memmap",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0
  );

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  // カーネルファイルを読み込む処理
  EFI_FILE_PROTOCOL* kernel_file;
  root_dir->Open(
      root_dir, &kernel_file, L"\\kernel.elf",
      EFI_FILE_MODE_READ, 0);

  // \kernel.elfの12文字分のバイト数を確保する
  // EFI_FILE_INFOのバイト数にFileNameのバイト数が含まれていないため

  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];

  // カーネルファイルの情報を取得
  kernel_file->GetInfo(
      kernel_file, &gEfiFileInfoGuid,
      &file_info_size, file_info_buffer);

  /**
   * EFI_FILE_INFO
   * typedef struct {
   *   UINT64 Size, FileSize, PhysicalSize;
   *   EFI_TIME CreateTime, LastAccessTime, ModificationTime;
   *   UINT64 Attributel
   *   CHAR16 FileName[]; // 文字数が変わりうる文字列
   * } EFI_FILE_INFO
   */
  // http://manabu.quu.cc/up/3/e31745m1.htm
  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  // EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000; // カーネルのエントリーポイントが設定されているアドレス
  // gBS->AllocatePages(
  //   AllocateAddress,                      // メモリの確保の仕方 
  //   EfiLoaderData,                        // 確保するメモリ領域の種別
  //   (kernel_file_size + 0xfff) / 0x1000,  // 大きさ（ページサイズを上げるための調整）
  //   &kernel_base_addr                     // 確保したメモリ領域のアドレス
  // );
  // kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  // Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(
      AllocateAddress, EfiLoaderData,
      (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
  kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

  // ブートサービスを停止
  EFI_STATUS status;
  // map_keyを取得するが、map_keyが最新の状態でないとエラーが起こる
  // ということで1回目は確実にエラーになる
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"failed to get memory map: %r\n", status);
      while (1);
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while (1);
    }
  }

  // カーネルを起動
  // UFIの仕様的にカーネルの24バイトの位置から8バイト整数として書かれることになっている
  // 以下の行でkernel_base_addr + 24をアドレスとして扱えるようにentry_addrに渡している
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  entry_point();

  Print(L"All done\n");

  while (1);
  return EFI_SUCCESS;
}
