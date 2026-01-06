/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "config.h"

#include "fu-lenovo-accessory-ble-command.h"
#include "fu-lenovo-accessory-ble-device.h"

struct _FuLenovoAccessoryBleDevice {
	FuBluezDevice parent_instance;
};

G_DEFINE_TYPE(FuLenovoAccessoryBleDevice, fu_lenovo_accessory_ble_device, FU_TYPE_BLUEZ_DEVICE)
#define UUID_WRITE "c1d02501-2d1f-400a-95d2-6a2f7bca0c25"
#define UUID_READ  "c1d02502-2d1f-400a-95d2-6a2f7bca0c25"

static gboolean
fu_lenovo_accessory_ble_device_setup(FuDevice *device, GError **error)
{
	/*g_autoptr(GByteArray) buf_write = g_byte_array_new();
	g_autoptr(GByteArray) buf_read = NULL;
	g_autofree gchar *id_display = NULL;
	g_autofree gchar *id_logical = NULL;
	g_autoptr(FuContext) buf_context = NULL;
	id_display = fu_device_get_id_display(device);
	id_logical = fu_device_get_logical_id(device);
	g_print("id_display:%s\n", id_display);
	g_print("id_logical:%s\n", id_logical);
	g_print("physical_id:%s\n", fu_device_get_physical_id(device));
	g_print("backend_id:%s\n", fu_device_get_backend_id(device));
	g_print("proxy_guid:%s\n", fu_device_get_proxy_guid(device));
	g_print("priority:%d\n", fu_device_get_priority(device));
	GPtrArray *instance_ids = fu_device_get_instance_ids(device);
	for (guint i = 0; i < instance_ids->len; i++) {
		const gchar *instance_id = g_ptr_array_index(instance_ids, i);
		g_print("instance_id %d:%s\n", i, instance_id);
	}
	GPtrArray *guids = fu_device_get_guids(device);
	for (guint i = 0; i < guids->len; i++) {
		const gchar *guid = g_ptr_array_index(guids, i);
		g_print("instance_id %d:%s\n", i, guid);
	}
	g_byte_array_append(buf_write, data, sizeof(data));
	if (!fu_bluez_device_write(FU_BLUEZ_DEVICE(device), UUID_WRITE, buf_write, error)) {
		g_prefix_error(error, "failed to write version cmd: ");
		return FALSE;
	}
	buf_read = fu_bluez_device_read(FU_BLUEZ_DEVICE(device), UUID_READ, error);
	if (buf_read->len < 9) {
		g_set_error(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_READ,
			    "invalid response length: %u",
			    buf_read->len);
		return FALSE;
	}
	version_str = g_strdup_printf("%u.%u.%u",
				      (guint)buf_read->data[6],
				      (guint)buf_read->data[7],
				      (guint)buf_read->data[8]);
	fu_device_set_version(device, version_str);
	g_debug("Lenovo BLE version detected: %s", version_str);*/
	return TRUE;
}

/*static gboolean
fu_lenovo_accessory_ble_write_firmware(FuDevice *device,
				       FuFirmware *firmware,
				       FuProgress *progress,
				       FwupdInstallFlags flags,
				       GError **error)
{
	return TRUE;
}*/

static void
fu_lenovo_accessory_ble_device_init(FuLenovoAccessoryBleDevice *self)
{
}

static void
fu_lenovo_accessory_ble_device_class_init(FuLenovoAccessoryBleDeviceClass *klass)
{
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	device_class->setup = fu_lenovo_accessory_ble_device_setup;
	/*device_class->write_firmware = fu_lenovo_accessory_ble_write_firmware;*/
}
