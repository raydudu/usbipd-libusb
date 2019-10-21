/*
 * Copyright (C) 2003-2008 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "stub.h"

#include <usbip_debug.h>

static const char *get_trx_type_str(uint8_t type)
{
	switch (type) {
	case LIBUSB_TRANSFER_TYPE_CONTROL:
		return "cont";
	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
		return "isoc";
	case LIBUSB_TRANSFER_TYPE_BULK:
		return "bulk";
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
		return "intr";
	case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
		return "blks";
    default:
        return "????";
    }
}

static void get_endpoint_descs(struct libusb_config_descriptor *config,
	const struct libusb_endpoint_descriptor *in[],
	const struct libusb_endpoint_descriptor *out[], int max)
{
	int i, j, k;
	const struct libusb_interface *intf;
	const struct libusb_interface_descriptor *desc;
	const struct libusb_endpoint_descriptor *ep;
	uint8_t dir;
	int num_in = 0;
	int num_out = 0;

	for (k = 0; k < max; k++) {
		in[k] = NULL;
		out[k] = NULL;
	}
	for (i = 0; i < config->bNumInterfaces; i++) {
		intf = config->interface + i;
		for (j = 0; j < intf->num_altsetting; j++) {
			desc = intf->altsetting + j;
			for (k = 0; k < desc->bNumEndpoints; k++) {
				ep = desc->endpoint + k;
				dir = ep->bEndpointAddress & 0x80;
				if (dir == LIBUSB_ENDPOINT_IN
					&& num_in < max) {
					in[num_in++] = ep;
				} else if (dir == LIBUSB_ENDPOINT_OUT
					&& num_out < max) {
					out[num_out++] = ep;
				}
			}
		}
	}
}

static void usbip_dump_ep_max(const struct libusb_endpoint_descriptor *ep[],
			      char *buf)
{
	int i;

	for (i = 0; i < 16; i++) {
		sprintf(buf+(3*i), " %2u", (ep[i]) ?
			libusb_le16_to_cpu(ep[i]->wMaxPacketSize) : 0);
	}
}

static void usbip_dump_usb_device(struct libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	int config_acquired = 0;
	const struct libusb_endpoint_descriptor *in[16], *out[16];
	char buf[3*16+1];

	if (libusb_get_device_descriptor(dev, &desc))
		dev_err(dev, "fail to get desc");

	if (libusb_get_active_config_descriptor(dev, &config) == 0)
		config_acquired = 1;

	dev_dbg(dev, "addr(%d)",
		libusb_get_device_address(dev));
	/* TODO: device number, device path */
	/* TODO: Transaction Translator info, tt */

	/* TODO: Toggle */

	if (config_acquired) {
		get_endpoint_descs(config, in, out, 16);
		usbip_dump_ep_max(in, buf);
		dev_dbg(dev, "epmaxp_in   :%s", buf);
		usbip_dump_ep_max(out, buf);
		dev_dbg(dev, "epmaxp_out  :%s", buf);
	}

	/* TODO: bus pointer */
	dev_dbg(dev, "parent %p",
		libusb_get_parent(dev));

	/* TODO: all configs pointer, raw descs */
	dev_dbg(dev, "vendor:0x%x product:0x%x actconfig:%p",
		desc.idVendor, desc.idProduct, config);

	/* TODO: have_lang, have_langid */

	/* TODO: maxchild */

	if (config_acquired)
		libusb_free_config_descriptor(config);
}

static const char *get_request_recipient_str(uint8_t rt)
{
	uint8_t recip = get_recipient(rt);

	switch (recip) {
	case LIBUSB_RECIPIENT_DEVICE:
		return "DEVC";
	case LIBUSB_RECIPIENT_INTERFACE:
		return "INTF";
	case LIBUSB_RECIPIENT_ENDPOINT:
		return "ENDP";
	case LIBUSB_RECIPIENT_OTHER:
		return "OTHR";
    default:
        return "????";
    }
}

static const char *get_request_str(uint8_t req)
{
	switch (req) {
	case LIBUSB_REQUEST_GET_STATUS:
		return "GET_STATUS";
	case LIBUSB_REQUEST_CLEAR_FEATURE:
		return "CLEAR_FEAT";
	case LIBUSB_REQUEST_SET_FEATURE:
		return "SET_FEAT  ";
	case LIBUSB_REQUEST_SET_ADDRESS:
		return "SET_ADDRRS";
	case LIBUSB_REQUEST_GET_DESCRIPTOR:
		return "GET_DESCRI";
	case LIBUSB_REQUEST_SET_DESCRIPTOR:
		return "SET_DESCRI";
	case LIBUSB_REQUEST_GET_CONFIGURATION:
		return "GET_CONFIG";
	case LIBUSB_REQUEST_SET_CONFIGURATION:
		return "SET_CONFIG";
	case LIBUSB_REQUEST_GET_INTERFACE:
		return "GET_INTERF";
	case LIBUSB_REQUEST_SET_INTERFACE:
		return "SET_INTERF";
	case LIBUSB_REQUEST_SYNCH_FRAME:
		return "SYNC_FRAME";
    default:
	    return "????      ";
	}
}

static void usbip_dump_usb_ctrlrequest(struct libusb_device *dev,
		struct libusb_transfer *trx)
{
	struct libusb_control_setup *cmd =
		libusb_control_transfer_get_setup(trx);

	if (!cmd) {
		dev_dbg(dev, "null control");
		return;
	}

	dev_dbg(dev, "bRequestType(%02X) bRequest(%02X) ",
		cmd->bmRequestType, cmd->bRequest);
	dev_dbg(dev, "wValue(%04X) wIndex(%04X) wLength(%04X)",
		cmd->wValue, cmd->wIndex, cmd->wLength);

	switch (cmd->bmRequestType & USB_TYPE_MASK) {
	case LIBUSB_REQUEST_TYPE_STANDARD:
		dev_dbg(dev, "STANDARD %s %s",
			get_request_str(cmd->bRequest),
			get_request_recipient_str(cmd->bmRequestType));
		break;
	case LIBUSB_REQUEST_TYPE_CLASS:
		dev_dbg(dev, "CLASS");
		break;
	case LIBUSB_REQUEST_TYPE_VENDOR:
		dev_dbg(dev, "VENDOR");
		break;
	case LIBUSB_REQUEST_TYPE_RESERVED:
		dev_dbg(dev, "RESERVED");
		break;
	}
}

void usbip_dump_trx(struct libusb_transfer *trx)
{
	struct libusb_device *dev;

	if (!trx) {
		dbg("trx: null pointer!!");
		return;
	}

	if (!trx->dev_handle) {
		dbg("trx->dev_handle: null pointer!!");
		return;
	}

	dev = libusb_get_device(trx->dev_handle);

	dev_dbg(dev, "   trx          :%p", trx);
	dev_dbg(dev, "   dev_handle   :%p", trx->dev_handle);
	dev_dbg(dev, "   trx_type     :%s", get_trx_type_str(trx->type));
	dev_dbg(dev, "   endpoint     :%08x", trx->endpoint);
	dev_dbg(dev, "   status       :%d", trx->status);
	dev_dbg(dev, "   trx_flags    :%08X", trx->flags);
	dev_dbg(dev, "   buffer       :%p", trx->buffer);
	dev_dbg(dev, "   buffer_length:%d", trx->length);
	dev_dbg(dev, "   actual_length:%d", trx->actual_length);
	dev_dbg(dev, "   num_packets  :%d", trx->num_iso_packets);
	dev_dbg(dev, "   context      :%p", trx->user_data);
	dev_dbg(dev, "   complete     :%p", trx->callback);

	if (trx->type == LIBUSB_TRANSFER_TYPE_CONTROL)
		usbip_dump_usb_ctrlrequest(dev, trx);

	usbip_dump_usb_device(dev);
}

void usbip_dump_header(struct usbip_header *pdu)
{
	dbg("BASE: cmd %u seq %u devid %u dir %u ep %u",
		 pdu->base.command,
		 pdu->base.seqnum,
		 pdu->base.devid,
		 pdu->base.direction,
		 pdu->base.ep);

	switch (pdu->base.command) {
	case USBIP_CMD_SUBMIT:
		dbg("USBIP_CMD_SUBMIT: xflg %x xln %u sf %x #p %d iv %d",
			 pdu->u.cmd_submit.transfer_flags,
			 pdu->u.cmd_submit.transfer_buffer_length,
			 pdu->u.cmd_submit.start_frame,
			 pdu->u.cmd_submit.number_of_packets,
			 pdu->u.cmd_submit.interval);
		break;
	case USBIP_CMD_UNLINK:
		dbg("USBIP_CMD_UNLINK: seq %u",
			 pdu->u.cmd_unlink.seqnum);
		break;
	case USBIP_RET_SUBMIT:
		dbg("USBIP_RET_SUBMIT: st %d al %u sf %x #p %d ec %d",
			 pdu->u.ret_submit.status,
			 pdu->u.ret_submit.actual_length,
			 pdu->u.ret_submit.start_frame,
			 pdu->u.ret_submit.number_of_packets,
			 pdu->u.ret_submit.error_count);
		break;
	case USBIP_RET_UNLINK:
		dbg("USBIP_RET_UNLINK: status %d",
			 pdu->u.ret_unlink.status);
		break;
	case USBIP_NOP:
		dbg("USBIP_NOP");
		break;
	default:
		/* NOT REACHED */
		err("unknown command");
		break;
	}
}

static int trxstat2error(enum libusb_transfer_status trxstat)
{
	switch (trxstat) {
	case LIBUSB_TRANSFER_COMPLETED:
		return 0;
	case LIBUSB_TRANSFER_CANCELLED:
		return -ECONNRESET;
	case LIBUSB_TRANSFER_STALL:
        return -EPIPE;
	case LIBUSB_TRANSFER_TIMED_OUT:
        return -ETIMEDOUT;
	case LIBUSB_TRANSFER_OVERFLOW:
		return -EOVERFLOW;
	case LIBUSB_TRANSFER_NO_DEVICE:
		return -ESHUTDOWN;
    case LIBUSB_TRANSFER_ERROR:
    default:
        return -ENOENT;
	}
}

static enum libusb_transfer_status error2trxstat(int e)
{
	switch (e) {
	case 0:
		return LIBUSB_TRANSFER_COMPLETED;
	case -ENOENT:
		return LIBUSB_TRANSFER_ERROR;
	case -ECONNRESET:
		return LIBUSB_TRANSFER_CANCELLED;
	case -ETIMEDOUT:
		return LIBUSB_TRANSFER_TIMED_OUT;
	case -EPIPE:
		return LIBUSB_TRANSFER_STALL;
	case -ESHUTDOWN:
		return LIBUSB_TRANSFER_NO_DEVICE;
	case -EOVERFLOW:
		return LIBUSB_TRANSFER_OVERFLOW;
    default:
        return LIBUSB_TRANSFER_ERROR;
    }
}

void usbip_pack_ret_submit(struct usbip_header *pdu,
			   struct libusb_transfer *trx)
{
	struct usbip_header_ret_submit *rpdu = &pdu->u.ret_submit;

	rpdu->status		= trxstat2error(trx->status);
	rpdu->actual_length	= trx->actual_length;
	rpdu->start_frame	= 0;
	rpdu->number_of_packets = trx->num_iso_packets;
	rpdu->error_count	= 0;
}

void usbip_pack_ret_unlink(struct usbip_header *pdu,
			   struct stub_unlink *unlink)
{
	struct usbip_header_ret_unlink *rpdu = &pdu->u.ret_unlink;

	rpdu->status = trxstat2error(unlink->status);
}

static void correct_endian_basic(struct usbip_header_basic *base, int send)
{
	if (send) {
		base->command	= htonl(base->command);
		base->seqnum	= htonl(base->seqnum);
		base->devid	= htonl(base->devid);
		base->direction	= htonl(base->direction);
		base->ep	= htonl(base->ep);
	} else {
		base->command	= ntohl(base->command);
		base->seqnum	= ntohl(base->seqnum);
		base->devid	= ntohl(base->devid);
		base->direction	= ntohl(base->direction);
		base->ep	= ntohl(base->ep);
	}
}

static void correct_endian_cmd_submit(struct usbip_header_cmd_submit *pdu,
				      int send)
{
	if (send) {
		pdu->transfer_flags = htonl(pdu->transfer_flags);

		pdu->transfer_buffer_length =
			(int32_t)htonl(pdu->transfer_buffer_length);
		pdu->start_frame = (int32_t)htonl(pdu->start_frame);
		pdu->number_of_packets = (int32_t)htonl(pdu->number_of_packets);
		pdu->interval = (int32_t)htonl(pdu->interval);
	} else {
		pdu->transfer_flags = ntohl(pdu->transfer_flags);

		pdu->transfer_buffer_length =
			(int32_t)ntohl(pdu->transfer_buffer_length);
		pdu->start_frame = (int32_t)ntohl(pdu->start_frame);
		pdu->number_of_packets = (int32_t)ntohl(pdu->number_of_packets);
		pdu->interval = (int32_t)ntohl(pdu->interval);
	}
}

static void correct_endian_ret_submit(struct usbip_header_ret_submit *pdu,
				      int send)
{
	if (send) {
		pdu->status = (int32_t)htonl(pdu->status);
		pdu->actual_length = (int32_t)htonl(pdu->actual_length);
		pdu->start_frame = (int32_t)htonl(pdu->start_frame);
		pdu->number_of_packets = (int32_t)htonl(pdu->number_of_packets);
		pdu->error_count = (int32_t)htonl(pdu->error_count);
	} else {
		pdu->status = (int32_t)ntohl(pdu->status);
		pdu->actual_length = (int32_t)ntohl(pdu->actual_length);
		pdu->start_frame = (int32_t)ntohl(pdu->start_frame);
		pdu->number_of_packets = (int32_t)ntohl(pdu->number_of_packets);
		pdu->error_count = (int32_t)ntohl(pdu->error_count);
	}
}

static void correct_endian_cmd_unlink(struct usbip_header_cmd_unlink *pdu,
				      int send)
{
	if (send)
		pdu->seqnum = htonl(pdu->seqnum);
	else
		pdu->seqnum = ntohl(pdu->seqnum);
}

static void correct_endian_ret_unlink(struct usbip_header_ret_unlink *pdu,
				      int send)
{
	if (send)
		pdu->status = (int32_t)htonl(pdu->status);
	else
		pdu->status = ntohl(pdu->status);
}

void usbip_header_correct_endian(struct usbip_header *pdu, int send)
{
	uint32_t cmd = 0;

	if (send)
		cmd = pdu->base.command;

	correct_endian_basic(&pdu->base, send);

	if (!send)
		cmd = pdu->base.command;

	switch (cmd) {
	case USBIP_CMD_SUBMIT:
		correct_endian_cmd_submit(&pdu->u.cmd_submit, send);
		break;
	case USBIP_RET_SUBMIT:
		correct_endian_ret_submit(&pdu->u.ret_submit, send);
		break;
	case USBIP_CMD_UNLINK:
		correct_endian_cmd_unlink(&pdu->u.cmd_unlink, send);
		break;
	case USBIP_RET_UNLINK:
		correct_endian_ret_unlink(&pdu->u.ret_unlink, send);
		break;
	case USBIP_NOP:
		break;
	default:
		err("unknown command");
		break;
	}
}

static void usbip_iso_packet_correct_endian(
		struct usbip_iso_packet_descriptor *iso, int send)
{
	/* does not need all members. but copy all simply. */
	if (send) {
		iso->offset	= htonl(iso->offset);
		iso->length	= htonl(iso->length);
		iso->status	= htonl(iso->status);
		iso->actual_length = htonl(iso->actual_length);
	} else {
		iso->offset	= ntohl(iso->offset);
		iso->length	= ntohl(iso->length);
		iso->status	= ntohl(iso->status);
		iso->actual_length = ntohl(iso->actual_length);
	}
}

static void usbip_pack_iso(struct usbip_iso_packet_descriptor *iso,
			   struct libusb_iso_packet_descriptor *uiso,
			   int offset, int pack)
{
	if (pack) {
		iso->offset		= offset;
		iso->length		= uiso->length;
		iso->status		= trxstat2error(uiso->status);
		iso->actual_length	= uiso->actual_length;
	} else {
		/* ignore iso->offset; */
		uiso->length		= iso->length;
		uiso->status		= error2trxstat(iso->status);
		uiso->actual_length	= iso->actual_length;
	}
}

/* must free buffer */
struct usbip_iso_packet_descriptor*
usbip_alloc_iso_desc_pdu(struct libusb_transfer *trx, ssize_t *bufflen)
{
	struct usbip_iso_packet_descriptor *iso;
	int np = trx->num_iso_packets;
	ssize_t size = np * sizeof(*iso);
	int i, offset = 0;

	iso = (struct usbip_iso_packet_descriptor *)malloc(size);
	if (!iso)
		return NULL;

	for (i = 0; i < np; i++) {
		usbip_pack_iso(&iso[i], &trx->iso_packet_desc[i], offset, 1);
		usbip_iso_packet_correct_endian(&iso[i], 1);
		offset += trx->iso_packet_desc[i].length;
	}

	*bufflen = size;

	return iso;
}