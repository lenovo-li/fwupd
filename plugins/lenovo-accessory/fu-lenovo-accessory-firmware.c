/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-lenovo-accessory-firmware.h"

struct _FuLenovoAccessoryFirmware {
	FuFirmware parent_instance;
};

G_DEFINE_TYPE(FuLenovoAccessoryFirmware, fu_lenovo_accessory_firmware, FU_TYPE_FIRMWARE)

static gboolean
fu_lenovo_accessory_firmware_parse(FuFirmware *firmware,
				   GInputStream *stream,
				   FuFirmwareParseFlags flags,
				   GError **error)
{
	return TRUE;
}

static void
fu_lenovo_accessory_firmware_export(FuFirmware *firmware,
				    FuFirmwareExportFlags flags,
				    XbBuilderNode *bn)
{
}

static void
fu_lenovo_accessory_firmware_finalize(GObject *object)
{
	G_OBJECT_CLASS(fu_lenovo_accessory_firmware_parent_class)->finalize(object);
}

static void
fu_lenovo_accessory_firmware_init(FuLenovoAccessoryFirmware *self)
{
}

static void
fu_lenovo_accessory_firmware_class_init(FuLenovoAccessoryFirmwareClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);
	object_class->finalize = fu_lenovo_accessory_firmware_finalize;
	firmware_class->parse = fu_lenovo_accessory_firmware_parse;
	firmware_class->export = fu_lenovo_accessory_firmware_export;
}
