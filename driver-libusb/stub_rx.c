// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2003-2008 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 */

#include "stub.h"
#include <usbip_debug.h>

static int is_clear_halt_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req =
			libusb_control_transfer_get_setup(trx);
	uint8_t recip = get_recipient(req->bmRequestType);

	return (req->bRequest == LIBUSB_REQUEST_CLEAR_FEATURE) &&
		 (recip == LIBUSB_RECIPIENT_ENDPOINT) &&
		 (req->wValue == USB_ENDPOINT_HALT);
}

static int is_set_interface_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req =
			libusb_control_transfer_get_setup(trx);
	uint8_t recip = get_recipient(req->bmRequestType);

	return (req->bRequest == LIBUSB_REQUEST_SET_INTERFACE) &&
		(recip == LIBUSB_RECIPIENT_INTERFACE);
}

static int is_set_configuration_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req =
			libusb_control_transfer_get_setup(trx);
	uint8_t recip = get_recipient(req->bmRequestType);

	return (req->bRequest == LIBUSB_REQUEST_SET_CONFIGURATION) &&
		(recip == LIBUSB_RECIPIENT_DEVICE);
}

static int is_reset_device_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req =
			libusb_control_transfer_get_setup(trx);
	uint8_t request_type = get_request_type(req->bmRequestType);
	uint16_t value;
	uint16_t index;

	value = libusb_cpu_to_le16(req->wValue);
	index = libusb_cpu_to_le16(req->wIndex);

	if ((req->bRequest == LIBUSB_REQUEST_SET_FEATURE) &&
	    (request_type == 	(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER)) &&
	    (value == USB_PORT_FEAT_RESET)) {
		usbip_dbg_stub_rx("reset_device_cmd, port %u", index);
		return 1;
	}
	return 0;
}

static int tweak_clear_halt_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req = libusb_control_transfer_get_setup(trx);
	unsigned char target_endp;
	int ret;

	/*
	 * The stalled endpoint is specified in the wIndex value. The endpoint
	 * of the urb is the target of this clear_halt request (i.e., control
	 * endpoint).
	 *
	 * NOTE: endpoint value must be with direction for libusb.
	 */
	target_endp = libusb_le16_to_cpu(req->wIndex);

	ret = libusb_clear_halt(trx->dev_handle, target_endp);
	if (ret)
		dev_err(libusb_get_device(trx->dev_handle),
			"usb_clear_halt error: endp %d ret %d",
			target_endp, ret);
	else
		dev_info(libusb_get_device(trx->dev_handle),
			"usb_clear_halt done: endp %d",
			target_endp);

	return ret;
}

static int tweak_set_interface_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req;
	uint16_t alternate;
	uint16_t interface;
	int ret;

	req = libusb_control_transfer_get_setup(trx);
	alternate = libusb_le16_to_cpu(req->wValue);
	interface = libusb_le16_to_cpu(req->wIndex);

	usbip_dbg_stub_rx("set_interface: inf %u alt %u",
			  interface, alternate);

	ret = libusb_set_interface_alt_setting(trx->dev_handle,
			interface, alternate);
	if (ret)
		dev_err(libusb_get_device(trx->dev_handle),
			"usb_set_interface error: inf %u alt %u ret %d",
			interface, alternate, ret);
	else
		dev_info(libusb_get_device(trx->dev_handle),
			"usb_set_interface done: inf %u alt %u",
			interface, alternate);

	return ret;
}

static int tweak_set_configuration_cmd(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req;
	uint16_t config;

	req = libusb_control_transfer_get_setup(trx);
	config = libusb_le16_to_cpu(req->wValue);

	/*
	 * I have never seen a multi-config device. Very rare.
	 * For most devices, this will be called to choose a default
	 * configuration only once in an initialization phase.
	 *
	 * set_configuration may change a device configuration and its device
	 * drivers will be unbound and assigned for a new device configuration.
	 * This means this usbip driver will be also unbound when called, then
	 * eventually reassigned to the device as far as driver matching
	 * condition is kept.
	 *
	 * Unfortunately, an existing usbip connection will be dropped
	 * due to this driver unbinding. So, skip here.
	 * A user may need to set a special configuration value before
	 * exporting the device.
	 */
	//TODO not sure if it is the point here
	dev_info(libusb_get_device(trx->dev_handle),
		"usb_set_configuration %d ... skip!",
		config);

	return 0;
}

static int tweak_reset_device_cmd(struct libusb_transfer *trx)
{
	struct stub_priv *priv = (struct stub_priv *) trx->user_data;
	struct stub_device *sdev = priv->sdev;

	dev_info(libusb_get_device(trx->dev_handle), "usb_queue_reset_device");

	/*
	 * With the implementation of pre_reset and post_reset the driver no
	 * longer unbinds. This allows the use of synchronous reset.
	 */
	libusb_reset_device(sdev->dev_handle);

	return 0;
}

/*
 * clear_halt, set_interface, and set_configuration require special tricks.
 */
static void tweak_special_requests(struct libusb_transfer *trx)
{
	struct libusb_control_setup *req;

	if (!trx)
		return;

	req = libusb_control_transfer_get_setup(trx);
	if (!req)
		return;

	if (trx->type != LIBUSB_TRANSFER_TYPE_CONTROL)
		return;

	if (is_clear_halt_cmd(trx))
		tweak_clear_halt_cmd(trx);

	else if (is_set_interface_cmd(trx))
		tweak_set_interface_cmd(trx);

	else if (is_set_configuration_cmd(trx))
		tweak_set_configuration_cmd(trx);

	else if (is_reset_device_cmd(trx))
		tweak_reset_device_cmd(trx);
	else
		usbip_dbg_stub_rx("no need to tweak");
}

/*
 * stub_recv_unlink() unlinks the URB by a call to usb_unlink_urb().
 * By unlinking the urb asynchronously, stub_rx can continuously
 * process coming urbs.  Even if the urb is unlinked, its completion
 * handler will be called and stub_tx will send a return pdu.
 *
 * See also comments about unlinking strategy in vhci_hcd.c.
 */
static int stub_recv_cmd_unlink(struct stub_device *sdev,
				struct usbip_header *pdu)
{
	int ret;
	struct list_head *pos;
	struct stub_priv *priv;

	pthread_mutex_lock(&sdev->priv_lock);

	list_for_each(pos, &sdev->priv_init) {
		priv = list_entry(pos, struct stub_priv, list);
		if (priv->seqnum != pdu->u.cmd_unlink.seqnum)
			continue;

		dev_info(libusb_get_device(priv->trx->dev_handle), "unlink urb %p", priv->trx);

		/*
		 * This matched urb is not completed yet (i.e., be in
		 * flight in usb hcd hardware/driver). Now we are
		 * cancelling it. The unlinking flag means that we are
		 * now not going to return the normal result pdu of a
		 * submission request, but going to return a result pdu
		 * of the unlink request.
		 */
		priv->unlinking = 1;

		/*
		 * In the case that unlinking flag is on, prev->seqnum
		 * is changed from the seqnum of the cancelling urb to
		 * the seqnum of the unlink request. This will be used
		 * to make the result pdu of the unlink request.
		 */
		priv->seqnum = pdu->base.seqnum;

		pthread_mutex_unlock(&sdev->priv_lock);

		/*
		 * usb_unlink_urb() is now out of spinlocking to avoid
		 * spinlock recursion since stub_complete() is
		 * sometimes called in this context but not in the
		 * interrupt context.  If stub_complete() is executed
		 * before we call usb_unlink_urb(), usb_unlink_urb()
		 * will return an error value. In this case, stub_tx
		 * will return the result pdu of this unlink request
		 * though submission is completed and actual unlinking
		 * is not executed. OK?
		 */
		/* In the above case, urb->status is not -ECONNRESET,
		 * so a driver in a client host will know the failure
		 * of the unlink request ?
		 */
		ret = libusb_cancel_transfer(priv->trx);
		if (ret == LIBUSB_ERROR_NOT_FOUND) {
			dev_err(libusb_get_device(priv->trx->dev_handle),
				"failed to unlink a urb completed urb%p",
				priv->trx);
		} else if (ret) {
			dev_err(libusb_get_device(priv->trx->dev_handle),
				"failed to unlink a urb %p, ret %d",
				priv->trx, ret);
		}
		return 0;
	}

	usbip_dbg_stub_rx("seqnum %d is not pending",
			  pdu->u.cmd_unlink.seqnum);

	/*
	 * The urb of the unlink target is not found in priv_init queue. It was
	 * already completed and its results is/was going to be sent by a
	 * CMD_RET pdu. In this case, usb_unlink_urb() is not needed. We only
	 * return the completeness of this unlink request to vhci_hcd.
	 */
	stub_enqueue_ret_unlink(sdev, pdu->base.seqnum, 0);

	pthread_mutex_unlock(&sdev->priv_lock);

	return 0;
}

static int valid_request(struct stub_device *sdev, struct usbip_header *pdu)
{
	struct usbip_device *ud = &sdev->ud;
	int valid = 0;

	if (pdu->base.devid == sdev->devid) {
		pthread_mutex_lock(&ud->lock);
		if (ud->status == SDEV_ST_USED) {
			/* A request is valid. */
			valid = 1;
		}
		pthread_mutex_unlock(&ud->lock);
	}
	if (!valid) {
		dev_err(sdev->dev, "invalid request %08x:%08x(%d)",
			pdu->base.devid, sdev->devid, ud->status);
	}
	return valid;
}

static struct stub_priv *stub_priv_alloc(struct stub_device *sdev,
					 struct usbip_header *pdu)
{
	struct stub_priv *priv;
	struct usbip_device *ud = &sdev->ud;

	pthread_mutex_lock(&sdev->priv_lock);

	priv = (struct stub_priv *)calloc(1, sizeof(struct stub_priv));
	if (!priv) {
		dev_err(sdev->dev, "alloc stub_priv");
		pthread_mutex_unlock(&sdev->priv_lock);
		usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
		return NULL;
	}

	priv->seqnum = pdu->base.seqnum;
	priv->dir = pdu->base.direction;
	priv->sdev = sdev;

	/*
	 * After a stub_priv is linked to a list_head,
	 * our error handler can free allocated data.
	 */
	list_add(&priv->list, sdev->priv_init.prev);

	pthread_mutex_unlock(&sdev->priv_lock);

	return priv;
}

static void masking_bogus_flags(struct libusb_transfer *trx)
{
	struct stub_priv *priv = (struct stub_priv *)trx->user_data;
	struct stub_device *sdev = priv->sdev;
	int is_out;
	unsigned int allowed = 0;

	if (!trx || !trx->callback) /* FIXME: omit hcpriv */
		return;
	/* FIXME: ignore dev->state */

	if (trx->type == LIBUSB_TRANSFER_TYPE_CONTROL) {
		struct libusb_control_setup *setup =
			libusb_control_transfer_get_setup(trx);

		if (!setup)
			return;
		is_out = !(setup->bmRequestType & USB_DIR_IN) ||
			!setup->wLength;
	} else {
		is_out = stub_endpoint_dir_out(sdev, trx->endpoint);
	}

	/* enforce simple/standard policy */
	switch (trx->type) {
	case LIBUSB_TRANSFER_TYPE_BULK:
		if (is_out)
			allowed |= LIBUSB_TRANSFER_ADD_ZERO_PACKET;
		/* FALLTHROUGH */
	case LIBUSB_TRANSFER_TYPE_CONTROL:
		/*allowed |= URB_NO_FSBR; */ /* only affects UHCI */
		/* FALLTHROUGH */
	default:			/* all non-iso endpoints */
		if (!is_out)
			allowed |= LIBUSB_TRANSFER_SHORT_NOT_OK;
		break;
	}
	trx->flags &= allowed;
}

static void stub_recv_cmd_submit(struct stub_device *sdev,
				 struct usbip_header *pdu)
{
	int ret;
	struct stub_priv *priv;
	struct usbip_device *ud = &sdev->ud;
	struct libusb_device_handle *dev_handle = sdev->dev_handle;
	unsigned char endpoint = pdu->base.ep;
	unsigned char trx_type = stub_get_transfer_type(sdev, pdu->base.ep);
	uint8_t trx_flags = stub_get_transfer_flags(
					pdu->u.cmd_submit.transfer_flags);
	int num_iso_packets = 0;
	struct libusb_transfer *trx;
	unsigned char *buf = NULL;
	int buflen = 0;
	int offset = 0;

	if (trx_type > LIBUSB_TRANSFER_TYPE_MASK)
		return;
	if (pdu->base.direction == USBIP_DIR_IN)
		endpoint |= USB_DIR_IN;

	priv = stub_priv_alloc(sdev, pdu);
	if (!priv)
		return;

	/* setup a urb */
	if (trx_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
		num_iso_packets = pdu->u.cmd_submit.number_of_packets;
	trx = libusb_alloc_transfer(num_iso_packets);
	if (!trx) {
		dev_err(sdev->dev, "malloc trx");
		usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
		return;
	}
	priv->trx = trx;

	/* allocate urb transfer buffer, if needed */
	if (trx_type == LIBUSB_TRANSFER_TYPE_CONTROL) {
		buflen = 8;
		offset = 8;
	}

	if (pdu->u.cmd_submit.transfer_buffer_length > 0)
		buflen += pdu->u.cmd_submit.transfer_buffer_length;

	if (buflen > 0) {
		buf = (unsigned char *)calloc(1, buflen);
		if (!buf) {
			usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
			return;
		}
	}

	/* copy urb setup packet */
	if (trx_type == LIBUSB_TRANSFER_TYPE_CONTROL)
		memcpy(buf, pdu->u.cmd_submit.setup, 8);

	/* set members from the base header of pdu */
	trx->dev_handle = dev_handle;
	trx->flags = trx_flags;
	trx->endpoint = endpoint;
	trx->type = trx_type;
	trx->timeout = 0;
	trx->buffer = buf;
	trx->length = buflen;
	trx->num_iso_packets = num_iso_packets;
	trx->user_data = priv;
	trx->callback = stub_complete;

	if (pdu->base.direction != USBIP_DIR_IN) {
		if (usbip_recv_xbuff(ud, trx, offset) < 0)
			return;
	}

	if (usbip_recv_iso(ud, trx) < 0)
		return;

	/* no need to submit an intercepted request, but harmless? */
	tweak_special_requests(trx);

	masking_bogus_flags(trx);

	/* urb is now ready to submit */
	ret = libusb_submit_transfer(priv->trx);

	if (ret == 0)
		usbip_dbg_stub_rx("submit_urb ok %p seq %u", trx, pdu->base.seqnum);
	else {
		dev_err(sdev->dev, "submit_urb error, %d  seq %u", ret, pdu->base.seqnum);
		usbip_dump_header(pdu);
		usbip_dump_trx(trx);
		libusb_free_transfer(trx);

		/*
		 * Pessimistic.
		 * This connection will be discarded.
		 */
		usbip_event_add(ud, SDEV_EVENT_ERROR_SUBMIT);
	}
}

/* recv a pdu */
static void stub_rx_pdu(struct usbip_device *ud)
{
	int ret;
	struct usbip_header pdu;
	struct stub_device *sdev = container_of(ud, struct stub_device, ud);

again:
	memset(&pdu, 0, sizeof(pdu));

	/* receive a pdu header */
	ret = usbip_recv(ud, &pdu, sizeof(pdu));

	if (ret != sizeof(pdu)) {
	    if (ret != 0) {
            dev_err(sdev->dev, "recv a header, %d", ret);
        } else {
	        dev_info(sdev->dev, "disconnect from client");
	    }
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		return;
	}

	usbip_header_correct_endian(&pdu, 0);

	if (usbip_dbg_flag_stub_rx)
		usbip_dump_header(&pdu);

	if (pdu.base.command == USBIP_NOP) {
		usbip_dbg_stub_rx("nop command");
		goto again;
	}

	if (!valid_request(sdev, &pdu)) {
		dev_err(sdev->dev, "recv invalid request");
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		return;
	}

	switch (pdu.base.command) {
	case USBIP_CMD_UNLINK:
		stub_recv_cmd_unlink(sdev, &pdu);
		break;
	case USBIP_CMD_SUBMIT:
		stub_recv_cmd_submit(sdev, &pdu);
		break;
	default:
		/* NOTREACHED */
		dev_err(sdev->dev, "unknown pdu");
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		break;
	}
}

void *stub_rx_loop(void *data)
{
	struct stub_device *sdev = (struct stub_device *)data;
	struct usbip_device *ud = &sdev->ud;

	while (!stub_should_stop(sdev)) {
		if (usbip_event_happened(ud))
			break;

		stub_rx_pdu(ud);
	}
	usbip_dbg_stub_rx("end of stub_rx_loop");
	return NULL;
}
