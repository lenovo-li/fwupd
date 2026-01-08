/*
 * Copyright 2026 Yuchao Li <liyc44@lenovo.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-lenovo-accessory-hid-command.h"
#include "fu-lenovo-accessory-struct.h"

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
	/*my_manual_dump("DEBUG WRITE", req, req_sz);*/
	if (!fu_hidraw_device_set_feature(hidraw_device, req, req_sz, flags, error))
		return FALSE;
	/* poll the device until the command is processed or we timeout */
	while (retries--) {
		/* get response using a GetFeature report */
		if (!fu_hidraw_device_get_feature(hidraw_device, rsp, req_sz, flags, error))
			return FALSE;
		/*my_manual_dump("DEBUG READ",rsp, req_sz);*/
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
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidFwVersion) lenovo_hid_fwversion = fu_lenovo_hid_fw_version_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x03);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_FIRMWARE_VERSION |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_fw_version_set_reportid(lenovo_hid_fwversion, 0x00);
	if (!fu_lenovo_hid_fw_version_set_cmd(lenovo_hid_fwversion, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_hid_command_process(hidraw_device,
						     lenovo_hid_fwversion->buf->data,
						     lenovo_hid_fwversion->buf->len,
						     FU_IOCTL_FLAG_RETRY,
						     error)) {
		return FALSE;
	}
	*major = fu_lenovo_hid_fw_version_get_major(lenovo_hid_fwversion);
	*minor = fu_lenovo_hid_fw_version_get_minor(lenovo_hid_fwversion);
	*internal = fu_lenovo_hid_fw_version_get_internal(lenovo_hid_fwversion);
	return TRUE;
}

gboolean
fu_lenovo_accessory_hid_command_dfu_set_devicemode(FuHidrawDevice *hidraw_device,
						   guint8 mode,
						   GError **error)
{
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidDevicemode) lenovo_hid_mode = fu_lenovo_hid_devicemode_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x01);
	fu_lenovo_accessory_cmd_set_command_class(
	    lenovo_hid_cmd,
	    FU_LENOVO_ACCESSORY_COMMAND_CLASS_DEVICE_INFORMATION);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_INFO_ID_DEVICE_MODE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_devicemode_set_reportid(lenovo_hid_mode, 0x00);
	if (!fu_lenovo_hid_devicemode_set_cmd(lenovo_hid_mode, lenovo_hid_cmd, error))
		return FALSE;
	fu_lenovo_hid_devicemode_set_mode(lenovo_hid_mode, mode);
	if (mode == 0x02) {
		return (fu_hidraw_device_set_feature(hidraw_device,
						     lenovo_hid_mode->buf->data,
						     lenovo_hid_mode->buf->len,
						     FU_IOCTL_FLAG_NONE,
						     error));
	} else {
		return fu_lenovo_accessory_hid_command_process(hidraw_device,
							       lenovo_hid_mode->buf->data,
							       lenovo_hid_mode->buf->len,
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
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidDfuExit) lenovo_hid_dfuexit = fu_lenovo_hid_dfu_exit_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_EXIT |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_dfu_exit_set_reportid(lenovo_hid_dfuexit, 0x00);
	if (!fu_lenovo_hid_dfu_exit_set_cmd(lenovo_hid_dfuexit, lenovo_hid_cmd, error))
		return FALSE;
	/*
	 * Note: fu_hidraw_device_set_feature() is guaranteed to return FALSE here.
	 * The device performs an immediate reset/reboot as soon as it receives the
	 * DFU_EXIT command and therefore never sends back an ACK. The resulting
	 * error (e.g., -EPIPE or -EIO) is expected and indicates that the reboot
	 * was successfully triggered.
	 */
	if (fu_hidraw_device_set_feature(hidraw_device,
					 lenovo_hid_dfuexit->buf->data,
					 lenovo_hid_dfuexit->buf->len,
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
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidDfuAttribute) lenovo_hid_attribute = fu_lenovo_hid_dfu_attribute_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_ATTRIBUTE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_GET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_dfu_attribute_set_reportid(lenovo_hid_attribute, 0x00);
	if (!fu_lenovo_hid_dfu_attribute_set_cmd(lenovo_hid_attribute, lenovo_hid_cmd, error))
		return FALSE;
	if (!fu_lenovo_accessory_hid_command_process(hidraw_device,
						     lenovo_hid_attribute->buf->data,
						     lenovo_hid_attribute->buf->len,
						     FU_IOCTL_FLAG_RETRY,
						     error))
		return FALSE;
	if (major_ver != NULL)
		*major_ver = fu_lenovo_hid_dfu_attribute_get_major_ver(lenovo_hid_attribute);
	if (minor_ver != NULL)
		*minor_ver = fu_lenovo_hid_dfu_attribute_get_minor_ver(lenovo_hid_attribute);
	if (product_pid != NULL)
		*product_pid = fu_lenovo_hid_dfu_attribute_get_product_pid(lenovo_hid_attribute);
	if (processor_id != NULL)
		*processor_id = fu_lenovo_hid_dfu_attribute_get_processor_id(lenovo_hid_attribute);
	if (app_max_size != NULL)
		*app_max_size = fu_lenovo_hid_dfu_attribute_get_app_max_size(lenovo_hid_attribute);
	if (page_size != NULL)
		*page_size = fu_lenovo_hid_dfu_attribute_get_page_size(lenovo_hid_attribute);
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
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidDfuPrepare) lenovo_hid_prepare = fu_lenovo_hid_dfu_prepare_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, 0x0D);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_PREPARE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_dfu_prepare_set_reportid(lenovo_hid_prepare, 0x00);
	if (!fu_lenovo_hid_dfu_prepare_set_cmd(lenovo_hid_prepare, lenovo_hid_cmd, error))
		return FALSE;
	fu_lenovo_hid_dfu_prepare_set_file_type(lenovo_hid_prepare, file_type);
	fu_lenovo_hid_dfu_prepare_set_start_address(lenovo_hid_prepare, start_address);
	fu_lenovo_hid_dfu_prepare_set_end_address(lenovo_hid_prepare, end_address);
	fu_lenovo_hid_dfu_prepare_set_crc32(lenovo_hid_prepare, crc32);
	return fu_lenovo_accessory_hid_command_process(hidraw_device,
						       lenovo_hid_prepare->buf->data,
						       lenovo_hid_prepare->buf->len,
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
	g_autoptr(FuLenovoAccessoryCmd) lenovo_hid_cmd = fu_lenovo_accessory_cmd_new();
	g_autoptr(FuLenovoHidDfuFw) lenovo_hid_fw = fu_lenovo_hid_dfu_fw_new();
	fu_lenovo_accessory_cmd_set_target_status(lenovo_hid_cmd, 0x00);
	fu_lenovo_accessory_cmd_set_data_size(lenovo_hid_cmd, block_size + 5);
	fu_lenovo_accessory_cmd_set_command_class(lenovo_hid_cmd,
						  FU_LENOVO_ACCESSORY_COMMAND_CLASS_DFU_CLASS);
	fu_lenovo_accessory_cmd_set_command_id(lenovo_hid_cmd,
					       FU_LENOVO_ACCESSORY_DFU_ID_DFU_FILE |
						   (FU_LENOVO_ACCESSORY_CMD_DIR_CMD_SET << 7));
	fu_lenovo_accessory_cmd_set_flag_profile(lenovo_hid_cmd, 0x00);
	fu_lenovo_hid_dfu_fw_set_reportid(lenovo_hid_fw, 0x00);
	if (!fu_lenovo_hid_dfu_fw_set_cmd(lenovo_hid_fw, lenovo_hid_cmd, error))
		return FALSE;
	fu_lenovo_hid_dfu_fw_set_file_type(lenovo_hid_fw, file_type);
	fu_lenovo_hid_dfu_fw_set_offset_address(lenovo_hid_fw, address);
	if (!fu_lenovo_hid_dfu_fw_set_data(lenovo_hid_fw, file_data, block_size, error))
		return FALSE;
	return fu_lenovo_accessory_hid_command_process(hidraw_device,
						       lenovo_hid_fw->buf->data,
						       lenovo_hid_fw->buf->len,
						       FU_IOCTL_FLAG_RETRY,
						       error);
}
