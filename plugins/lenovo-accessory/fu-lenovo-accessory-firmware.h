/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_LENOVO_ACCESSORY_FIRMWARE (fu_lenovo_accessory_firmware_get_type())
G_DECLARE_FINAL_TYPE(FuLenovoAccessoryFirmware,
		     fu_lenovo_accessory_firmware,
		     FU,
		     LENOVO_ACCESSORY_FIRMWARE,
		     FuFirmware)

struct _FuLenovoAccessoryFirmwareClass {
	FuFirmwareClass parent_class;
};
