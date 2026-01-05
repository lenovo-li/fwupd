#[repr(u8)]
enum FuLenovoHidDataHeader {
    TargetStatus = 0x01,
    DataSize = 0x02,
    CommandClass = 0x03,
    CommandId = 0x04,
    FlagProfile = 0x05,
    Reserved = 0x06,
    PayloadBase = 0x07,
}

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
enum FuLenovoHidCmdDir {
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
struct FuLenovoHidCmd{
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
    cmd:FuLenovoHidCmd,
    data: [u8; 58],
}
