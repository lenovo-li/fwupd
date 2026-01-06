#[repr(u8)]
enum FuLenovoAccessoryCommandClass {
    DeviceInformation = 0x00,
    DfuClass = 0x09,
}

#[repr(u8)]
enum FuLenovoAccessoryInfoId {
   FirmwareVersion = 0x01,
   DeviceMode = 0x04,
}

#[repr(u8)]
enum FuLenovoAccessoryDfuId {
    DfuAttribute = 0x01,
    DfuPrepare = 0x02,
    DfuFile = 0x03,
    DfuCrc = 0x04,
    DfuExit = 0x05,
    DfuEntry = 0x06,
}

#[repr(u8)]
enum FuLenovoAccessoryCmdDir {
    CmdSet = 0x00,
    CmdGet = 0x01,
}

#[repr(u8)]
enum FuLenovoStatus{
    NewCommand = 0x00,
    CommandBusy = 0x01,
    CommandSuccessful = 0x02,
    CommandFailure = 0x03,
    CommandTimeOut = 0x04,
    CommandNotSupport = 0x05,
}

#[derive(New,Validate,Parse,Default)]
#[repr(C,packed)]
struct FuLenovoAccessoryCmd{
    target_status:u8,
    data_size:u8,
    command_class:u8,
    command_id:u8,
    flag_profile:u8,
    reserved:u8,
}

#[derive(New,Validate,Parse,Default)]
#[repr(C,packed)]
struct FuLenovoHidData{
    reportid:u8,
    cmd:FuLenovoAccessoryCmd,
    data: [u8; 58],
}

#[derive(New,Validate,Parse,Default)]
#[repr(C,packed)]
struct FuLenovoBleData{
    cmd:FuLenovoAccessoryCmd,
    data: [u8; 58],
}
