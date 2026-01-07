/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-lenovo-accessory-hid-command.h"
#include "fu-lenovo-accessory-struct.h"

static gboolean
fu_lenovo_accessory_hid_command_process(FuHidrawDevice *hidraw_device,
					guint8 *req,
					gsize req_sz,
					FuIoctlFlags flags,
					GError **error)
{
	g_autofree guint8 *rsp = g_malloc0(req_sz);
	guint retries = 5;
	guint8 status = 0;
	/* send request to the device using a SetFeature report */
	if (!fu_hidraw_device_set_feature(hidraw_device, req, req_sz, flags, error))
		return FALSE;
	/* poll the device until the command is processed or we timeout */
	while (retries--) {
		/* get response using a GetFeature report */
		if (!fu_hidraw_device_get_feature(hidraw_device, rsp, req_sz, flags, error))
			return FALSE;
		status = rsp[1] & 0x0F;
		if (status == FU_LENOVO_STATUS_COMMAND_SUCCESSFUL) {
			if (!fu_memcpy_safe(req, req_sz, 0, rsp, req_sz, 0, req_sz, error))
				return FALSE;
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
		/* wait 10ms before retrying */
		g_usleep(10 * 1000);
	}

	g_set_error_literal(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_WRITE,
			    "command timeout (device always busy)");
	return FALSE;
}

gboolean
fu_lenovo_accessory_hid_command_fwversion(FuHidrawDevice *hidraw_device,
					  guint8 *major,
					  guint8 *minor,
					  guint8 *internal,
					  GError **error)
{
	const guint8 *receive_data = NULL;
	gsize bufz = 0;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x03);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_FIRMWARE_VERSION |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_hid_command_process(hidraw_device,
						     lenovo_hid_data->buf->data,
						     lenovo_hid_data->buf->len,
						     FU_IOCTL_FLAG_RETRY,
						     error)) {
		return FALSE;
	}
	receive_data = fu_lenovo_hid_data_get_data(lenovo_hid_data, &bufz);
	*major = receive_data[0];
	*minor = receive_data[1];
	*internal = receive_data[2];
	return TRUE;
}

gboolean
fu_lenovo_accessory_hid_command_dfu_set_devicemode(FuHidrawDevice *hidraw_device,
						   guint8 mode,
						   GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x01);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_DEVICE_MODE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_hid_data_set_data(lenovo_hid_data, &mode, 1, error))
		return FALSE;
	if (mode == 0x02) {
		return (fu_hidraw_device_set_feature(hidraw_device,
						     lenovo_hid_data->buf->data,
						     lenovo_hid_data->buf->len,
						     FU_IOCTL_FLAG_NONE,
						     error));
	} else {
		return fu_lenovo_accessory_hid_command_process(hidraw_device,
							       lenovo_hid_data->buf->data,
							       lenovo_hid_data->buf->len,
							       FU_IOCTL_FLAG_RETRY,
							       error);
	}
}

/**
 * fu_lenovo_accessory_hid_command_dfu_exit:
 * @hidraw_device: a #FuHidrawDevice
 * @exit_code: the exit status code (e.g., 0x00 for success/reboot)
 *
 * Sends the DFU_EXIT command to the device to finalize the update.
 * Since this command triggers an immediate hardware reset/reboot, the
 * device will disconnect from the USB bus before it can send an ACK.
 * Consequently, the set_feature call is expected to return an error
 * (e.g., Broken pipe or I/O error), which we intentionally ignore.
 */
gboolean
fu_lenovo_accessory_hid_command_dfu_exit(FuHidrawDevice *hidraw_device,
					 guint8 exit_code,
					 GError **error)
{
	g_autoptr(GError) error_local = NULL;
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_EXIT |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	/*
	 * Note: fu_hidraw_device_set_feature() is guaranteed to return FALSE here.
	 * The device performs an immediate reset/reboot as soon as it receives the
	 * DFU_EXIT command and therefore never sends back an ACK. The resulting
	 * error (e.g., -EPIPE or -EIO) is expected and indicates that the reboot
	 * was successfully triggered.
	 */
	if (fu_hidraw_device_set_feature(hidraw_device,
					 lenovo_hid_data->buf->data,
					 lenovo_hid_data->buf->len,
					 FU_IOCTL_FLAG_NONE,
					 error))
		return TRUE;
	return TRUE;
}

gboolean
fu_lenovo_accessory_hid_command_dfu_attribute(FuHidrawDevice *hidraw_device,
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
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_ATTRIBUTE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_hid_command_process(hidraw_device,
						     lenovo_hid_data->buf->data,
						     lenovo_hid_data->buf->len,
						     FU_IOCTL_FLAG_RETRY,
						     error))
		return FALSE;
	receive_data = fu_lenovo_hid_data_get_data(lenovo_hid_data, &bufz);
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
fu_lenovo_accessory_hid_command_dfu_prepare(FuHidrawDevice *hidraw_device,
					    guint8 file_type,
					    guint32 start_address,
					    guint32 end_address,
					    guint32 crc32,
					    GError **error)
{
	g_autofree guint8 *send_data = g_new0(guint8, 13);
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_PREPARE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	send_data[0] = file_type;
	fu_memwrite_uint32(send_data + 1, start_address, G_BIG_ENDIAN);
	fu_memwrite_uint32(send_data + 5, end_address, G_BIG_ENDIAN);
	fu_memwrite_uint32(send_data + 9, crc32, G_BIG_ENDIAN);
	if (!fu_lenovo_hid_data_set_data(lenovo_hid_data, send_data, 13, error))
		return FALSE;
	return fu_lenovo_accessory_hid_command_process(hidraw_device,
						       lenovo_hid_data->buf->data,
						       lenovo_hid_data->buf->len,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}

gboolean
fu_lenovo_accessory_hid_command_dfu_file(FuHidrawDevice *hidraw_device,
					 guint8 file_type,
					 guint32 address,
					 const guint8 *file_data,
					 guint8 block_size,
					 GError **error)
{
	g_autofree guint8 *send_data = g_new0(guint8, 65);
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidData) lenovo_hid_data = fu_lenovo_hid_data_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, block_size + 5);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_FILE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_data_set_reportid(lenovo_hid_data, 0x00);
	if (!fu_lenovo_hid_data_set_cmd(lenovo_hid_data, lenovo_hid_cmd, error))
		return FALSE;
	send_data[0] = file_type;
	fu_memwrite_uint32(send_data + 1, address, G_BIG_ENDIAN);
	if (!fu_memcpy_safe(send_data + 5, 58, 0, file_data, 58, 0, block_size, error))
		return FALSE;
	if (!fu_lenovo_hid_data_set_data(lenovo_hid_data, send_data, 58, error))
		return FALSE;
	return fu_lenovo_accessory_hid_command_process(hidraw_device,
						       lenovo_hid_data->buf->data,
						       lenovo_hid_data->buf->len,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}
