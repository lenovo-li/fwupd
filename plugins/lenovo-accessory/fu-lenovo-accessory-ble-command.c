/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-lenovo-accessory-ble-command.h"
#include "fu-lenovo-accessory-struct.h"

#define UUID_WRITE "c1d02501-2d1f-400a-95d2-6a2f7bca0c25"
#define UUID_READ  "c1d02502-2d1f-400a-95d2-6a2f7bca0c25"

/*static void
my_manual_dump(const gchar *title, const guint8 *data, gsize len)
{
	g_print("%s [%" G_GSIZE_FORMAT " bytes]:\n", title, len);
	for (gsize i = 0; i < len; i++) {
		g_print("%02x ", data[i]);
		if ((i + 1) % 16 == 0)
			g_print("\n");
	}
	g_print("\n---------------------------\n");
}*/

static gboolean
fu_lenovo_accessory_ble_command_process(FuBluezDevice *ble_device,
					GByteArray *buffer,
					FuIoctlFlags flags,
					GError **error)
{
	guint retries = 50;
	/*my_manual_dump("DEBUG WRITE", buffer->data, buffer->len);*/
	if (!fu_bluez_device_write(ble_device, UUID_WRITE, buffer, error)) {
		g_prefix_error_literal(error, "failed to write cmd: ");
		return FALSE;
	}

	while (retries--) {
		guint8 status = 0;
		g_autoptr(GByteArray) res = fu_bluez_device_read(ble_device, UUID_READ, error);
		if (res == NULL)
			return FALSE;
		/*my_manual_dump("DEBUG READ", res->data, res->len);*/
		if (res->len < 1) {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_READ,
					    "received empty data");
			return FALSE;
		}

		status = res->data[0] & 0x0F;

		if (status == FU_LENOVO_STATUS_COMMAND_SUCCESSFUL) {
			g_byte_array_set_size(buffer, 0);
			g_byte_array_append(buffer, res->data, res->len);
			return TRUE;
		}

		if (status != FU_LENOVO_STATUS_COMMAND_BUSY) {
			g_set_error(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_WRITE,
				    "command failed with status 0x%02x",
				    status);
			return FALSE;
		}
		g_usleep(10 * 1000);
	}
	g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_WRITE, "timeout (busy)");
	return FALSE;
}

gboolean
fu_lenovo_accessory_ble_command_fwversion(FuBluezDevice *ble_device,
					  guint8 *major,
					  guint8 *minor,
					  guint8 *internal,
					  GError **error)
{
	gsize bufz = 0;
	const guint8 *receive_data = NULL;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x03);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_FIRMWARE_VERSION |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_ble_command_process(ble_device,
						     lenovo_ble_data->buf,
						     FU_IOCTL_FLAG_RETRY,
						     error))
		return FALSE;
	receive_data = fu_lenovo_ble_data_get_data(lenovo_ble_data, &bufz);
	if (receive_data == NULL || bufz < 3) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_READ,
				    "invalid response length");
		return FALSE;
	}

	if (major != NULL)
		*major = receive_data[0];
	if (minor != NULL)
		*minor = receive_data[1];
	if (internal != NULL)
		*internal = receive_data[2];
	return TRUE;
}

gboolean
fu_lenovo_accessory_ble_command_get_devicemode(FuBluezDevice *ble_device,
					       guint8 *mode,
					       GError **error)
{
	gsize bufsz = 0;
	const guint8 *receive_data = NULL;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x01);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_DEVICE_MODE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_ble_command_process(ble_device,
						     lenovo_ble_data->buf,
						     FU_IOCTL_FLAG_RETRY,
						     error)) {
		return FALSE;
	}
	receive_data = fu_lenovo_ble_data_get_data(lenovo_ble_data, &bufsz);
	if (receive_data == NULL || bufsz == 0)
		return FALSE;
	*mode = receive_data[0];
	return TRUE;
}

gboolean
fu_lenovo_accessory_ble_command_dfu_set_devicemode(FuBluezDevice *ble_device,
						   guint8 mode,
						   GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x01);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_DEVICE_MODE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_ble_data_set_data(lenovo_ble_data, &mode, 1, error))
		return FALSE;
	if (mode == 0x02)
		return fu_bluez_device_write(ble_device, UUID_WRITE, lenovo_ble_data->buf, error);
	return fu_lenovo_accessory_ble_command_process(ble_device,
						       lenovo_ble_data->buf,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}

gboolean
fu_lenovo_accessory_ble_command_dfu_exit(FuBluezDevice *ble_device,
					 guint8 exit_code,
					 GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x01);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_EXIT |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_ble_data_set_data(lenovo_ble_data, &exit_code, 1, error))
		return FALSE;
	if (!fu_bluez_device_write(ble_device, UUID_WRITE, lenovo_ble_data->buf, error)) {
		g_prefix_error_literal(error, "failed to write cmd: ");
		return FALSE;
	}
	return TRUE;
}

gboolean
fu_lenovo_accessory_ble_command_dfu_attribute(FuBluezDevice *ble_device,
					      guint8 *major_ver,
					      guint8 *minor_ver,
					      guint16 *product_pid,
					      guint8 *processor_id,
					      guint32 *app_max_size,
					      guint32 *page_size,
					      GError **error)
{
	gsize bufz = 0;
	const guint8 *receive_data = NULL;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_ATTRIBUTE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;

	if (!fu_lenovo_accessory_ble_command_process(ble_device,
						     lenovo_ble_data->buf,
						     FU_IOCTL_FLAG_RETRY,
						     error))
		return FALSE;

	receive_data = fu_lenovo_ble_data_get_data(lenovo_ble_data, &bufz);
	if (receive_data == NULL || bufz < 13) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_READ,
				    "invalid attribute length");
		return FALSE;
	}

	if (major_ver != NULL)
		*major_ver = receive_data[0];
	if (minor_ver != NULL)
		*minor_ver = receive_data[1];
	if (product_pid != NULL)
		*product_pid = fu_memread_uint16(receive_data + 2, G_BIG_ENDIAN);
	if (processor_id != NULL)
		*processor_id = receive_data[4];
	if (app_max_size != NULL)
		*app_max_size = fu_memread_uint32(receive_data + 5, G_BIG_ENDIAN);
	if (page_size != NULL)
		*page_size = fu_memread_uint32(receive_data + 9, G_BIG_ENDIAN);

	return TRUE;
}

gboolean
fu_lenovo_accessory_ble_command_dfu_prepare(FuBluezDevice *ble_device,
					    guint8 file_type,
					    guint32 start_address,
					    guint32 end_address,
					    guint32 crc32,
					    GError **error)
{
	guint8 send_data[13] = {0};
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_PREPARE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;

	send_data[0] = file_type;
	fu_memwrite_uint32(send_data + 1, start_address, G_BIG_ENDIAN);
	fu_memwrite_uint32(send_data + 5, end_address, G_BIG_ENDIAN);
	fu_memwrite_uint32(send_data + 9, crc32, G_BIG_ENDIAN);

	if (!fu_lenovo_ble_data_set_data(lenovo_ble_data, send_data, 13, error))
		return FALSE;

	return fu_lenovo_accessory_ble_command_process(ble_device,
						       lenovo_ble_data->buf,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}

gboolean
fu_lenovo_accessory_ble_command_dfu_file(FuBluezDevice *ble_device,
					 guint8 file_type,
					 guint32 address,
					 const guint8 *file_data,
					 guint8 block_size,
					 GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleDfuFw) lenovo_ble_file = fu_lenovo_ble_dfu_fw_new();

	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, block_size + 5);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_FILE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_dfu_fw_set_cmd(lenovo_ble_file, lenovo_hid_cmd, error))
		return FALSE;
	fu_lenovo_ble_dfu_fw_set_file_type(lenovo_ble_file, file_type);
	fu_lenovo_ble_dfu_fw_set_offset_address(lenovo_ble_file, address);
	if (!fu_lenovo_ble_dfu_fw_set_data(lenovo_ble_file, file_data, block_size, error))
		return FALSE;
	return fu_lenovo_accessory_ble_command_process(ble_device,
						       lenovo_ble_file->buf,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}

gboolean
fu_lenovo_accessory_ble_command_dfu_crc(FuBluezDevice *ble_device, guint32 *crc32, GError **error)
{
	gsize bufz = 0;
	const guint8 *receive_data = NULL;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x05);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_CRC |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_ble_command_process(ble_device,
						     lenovo_ble_data->buf,
						     FU_IOCTL_FLAG_RETRY,
						     error))
		return FALSE;

	receive_data = fu_lenovo_ble_data_get_data(lenovo_ble_data, &bufz);
	if (receive_data == NULL || bufz < 4) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_READ,
				    "invalid attribute length");
		return FALSE;
	}
	if (crc32 != NULL)
		*crc32 = fu_memread_uint32(receive_data, G_BIG_ENDIAN);
	return TRUE;
}

gboolean
fu_lenovo_accessory_ble_command_dfu_entry(FuBluezDevice *ble_device, GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoBleData) lenovo_ble_data = fu_lenovo_ble_data_new();

	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_ENTRY |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));

	if (!fu_lenovo_ble_data_set_cmd(lenovo_ble_data, lenovo_hid_cmd, error))
		return FALSE;
	return fu_lenovo_accessory_ble_command_process(ble_device,
						       lenovo_ble_data->buf,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}
