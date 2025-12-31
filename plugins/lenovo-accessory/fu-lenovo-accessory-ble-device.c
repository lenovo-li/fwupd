#include "config.h"

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
	g_autoptr(GByteArray) buf_write = g_byte_array_new();
	g_autoptr(GByteArray) buf_read = NULL;
	g_autofree gchar *id_display = NULL;
	g_autofree gchar *context = NULL;
	g_autofree gchar *version_str = NULL;
	guint8 data[64] = {0};
	data[1] = 0x03;
	data[2] = 0x00;
	data[3] = 0x81;
	id_display = fu_device_get_id_display(device);
	context = fu_device_get_context(device);
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
	g_debug("Lenovo BLE version detected: %s", version_str);
	return TRUE;
}

static gboolean
fu_lenovo_accessory_ble_write_firmware(FuDevice *device,
				       FuFirmware *firmware,
				       FuProgress *progress,
				       FwupdInstallFlags flags,
				       GError **error)
{
	return TRUE;
}
static void
fu_lenovo_accessory_ble_device_init(FuLenovoAccessoryBleDevice *self)
{
}

static void
fu_lenovo_accessory_ble_device_class_init(FuLenovoAccessoryBleDeviceClass *klass)
{
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	device_class->setup = fu_lenovo_accessory_ble_device_setup;
	device_class->write_firmware = fu_lenovo_accessory_ble_write_firmware;
}
