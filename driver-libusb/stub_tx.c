// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2003-2008 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 */

#include "stub.h"
#include <usbip_debug.h>

static void stub_free_priv_and_trx(struct stub_priv *priv)
{
	struct libusb_transfer *trx = priv->trx;

	free(trx->buffer);
	list_del(&priv->list);
	free(priv);
	usbip_dbg_stub_tx("freeing trx %p", trx);
	libusb_free_transfer(trx);
}

/* be in spin_lock_irqsave(&sdev->priv_lock, flags) */
int stub_enqueue_ret_unlink(struct stub_device *sdev, uint32_t seqnum,
			     enum libusb_transfer_status status)
{
	struct stub_unlink *unlink;

	unlink = (struct stub_unlink *)calloc(1, sizeof(struct stub_unlink));
	if (!unlink) {
        return  -1;
	}

	unlink->seqnum = seqnum;
	unlink->status = status;

	list_add(&unlink->list, sdev->unlink_tx.prev);
    return 0;
}

/**
 * stub_complete - completion handler of a usbip urb
 * @urb: pointer to the urb completed
 *
 * When a urb has completed, the USB core driver calls this function mostly in
 * the interrupt context. To return the result of a urb, the completed urb is
 * linked to the pending list of returning.
 *
 */
void LIBUSB_CALL stub_complete(struct libusb_transfer *trx)
{
	struct stub_priv *priv = (struct stub_priv *) trx->user_data;
	struct stub_device *sdev = priv->sdev;

	usbip_dbg_stub_tx("complete %p! status %d", trx, trx->status);

	switch (trx->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		/* OK */
		break;
	case LIBUSB_TRANSFER_ERROR:
	    if (!(trx->flags & LIBUSB_TRANSFER_SHORT_NOT_OK)) {
            dev_info(sdev->dev, "error on endpoint %d", trx->endpoint);
        } else {
	        //Tweaking status to complete as we received data, but all
	        trx->status = LIBUSB_TRANSFER_COMPLETED;
	    }
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		dev_info(sdev->dev, "unlinked by a call to usb_unlink_urb()");
		break;
	case LIBUSB_TRANSFER_STALL:
		dev_err(sdev->dev, "endpoint %d is stalled", trx->endpoint);
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		dev_info(sdev->dev, "device removed?");
		break;
	default:
		dev_err(sdev->dev,  "urb completion with unknown status %d", trx->status);
		break;
	}

	/* link a urb to the queue of tx. */
	pthread_mutex_lock(&sdev->priv_lock);
	if (!sdev->ud.sock_fd) {
		dev_info(sdev->dev,
			"urb discarded in closed connection");
	} else if (priv->unlinking) {
		stub_enqueue_ret_unlink(sdev, priv->seqnum, trx->status);
		stub_free_priv_and_trx(priv);
	} else {
		list_del(&priv->list);
		list_add(&priv->list, sdev->priv_tx.prev);
	}
	pthread_mutex_unlock(&sdev->priv_lock);

	/* wake up tx_thread */
	pthread_mutex_unlock(&sdev->tx_waitq);
}

static inline void setup_base_pdu(struct usbip_header_basic *base,
				  uint32_t command, uint32_t seqnum)
{
	base->command	= command;
	base->seqnum	= seqnum;
	base->devid	= 0;
	base->ep	= 0;
	base->direction = 0;
}

static void setup_ret_submit_pdu(struct usbip_header *rpdu,
				 struct libusb_transfer *trx)
{
	struct stub_priv *priv = (struct stub_priv *) trx->user_data;

	setup_base_pdu(&rpdu->base, USBIP_RET_SUBMIT, priv->seqnum);
	usbip_pack_ret_submit(rpdu, trx);
}

static void setup_ret_unlink_pdu(struct usbip_header *rpdu,
				 struct stub_unlink *unlink)
{
	setup_base_pdu(&rpdu->base, USBIP_RET_UNLINK, unlink->seqnum);
	usbip_pack_ret_unlink(rpdu, unlink);
}

static struct stub_priv *dequeue_from_priv_tx(struct stub_device *sdev)
{
	struct list_head *pos, *tmp;
	struct stub_priv *priv;

	pthread_mutex_lock(&sdev->priv_lock);

	list_for_each_safe(pos, tmp, &sdev->priv_tx) {
		priv = list_entry(pos, struct stub_priv, list);
		list_del(&priv->list);
		list_add(&priv->list, sdev->priv_free.prev);
		pthread_mutex_unlock(&sdev->priv_lock);
		return priv;
	}

	pthread_mutex_unlock(&sdev->priv_lock);

	return NULL;
}

static void fixup_actual_length(struct libusb_transfer *trx)
{
	int i, len = 0;

	for (i = 0; i < trx->num_iso_packets; i++)
		len += trx->iso_packet_desc[i].actual_length;

	trx->actual_length = len;
}

static int stub_send_ret_submit(struct stub_device *sdev)
{
	struct list_head *pos, *tmp;
	struct stub_priv *priv;
	size_t total_size = 0;

	while ((priv = dequeue_from_priv_tx(sdev)) != NULL) {
		size_t sent;
		struct libusb_transfer *trx = priv->trx;
		struct usbip_header pdu_header;
		struct usbip_iso_packet_descriptor *iso_buffer = NULL;
		struct iovec iov[2 + trx->num_iso_packets];
		int iovnum = 0;
		int offset = 0;
		size_t txsize = 0;

		memset(&pdu_header, 0, sizeof(pdu_header));

		if (trx->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
			fixup_actual_length(trx);
		}

		/* 1. setup usbip_header */
		setup_ret_submit_pdu(&pdu_header, trx);
		usbip_dbg_stub_tx("setup txdata seqnum: %d trx: %p actl: %d",
			  pdu_header.base.seqnum, trx, trx->actual_length);
		usbip_header_correct_endian(&pdu_header, 1);

		iov[iovnum].iov_base = &pdu_header;
		iov[iovnum].iov_len  = sizeof(pdu_header);
		iovnum++;
		txsize += sizeof(pdu_header);

		/* 2. setup transfer buffer */
		if (priv->dir == USBIP_DIR_IN &&
			trx->type != LIBUSB_TRANSFER_TYPE_ISOCHRONOUS &&
			trx->actual_length > 0) {
			if (trx->type == LIBUSB_TRANSFER_TYPE_CONTROL)
				offset = 8;
			iov[iovnum].iov_base = trx->buffer + offset;
			iov[iovnum].iov_len  = trx->actual_length;
			iovnum++;
			txsize += trx->actual_length;
		} else if (priv->dir == USBIP_DIR_IN &&
			trx->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
			/*
			 * For isochronous packets: actual length is the sum of
			 * the actual length of the individual, packets, but as
			 * the packet offsets are not changed there will be
			 * padding between the packets. To optimally use the
			 * bandwidth the padding is not transmitted.
			 */
			int i;

			for (i = 0; i < trx->num_iso_packets; i++) {
				iov[iovnum].iov_base = trx->buffer + offset;
				iov[iovnum].iov_len =
					trx->iso_packet_desc[i].actual_length;
				iovnum++;
				offset += trx->iso_packet_desc[i].length;
				txsize += trx->iso_packet_desc[i].actual_length;
			}

			if (txsize != sizeof(pdu_header) + trx->actual_length) {
				dev_err(sdev->dev,
					"actual length of urb %d does not ",
					trx->actual_length);
				dev_err(sdev->dev,
					"match iso packet sizes %zu",
					txsize-sizeof(pdu_header));
				return -1;
			}
		}

		/* 3. setup iso_packet_descriptor */
		if (trx->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
			ssize_t len = 0;

			iso_buffer = usbip_alloc_iso_desc_pdu(trx, &len);
			if (!iso_buffer) {
				return -1;
			}

			iov[iovnum].iov_base = iso_buffer;
			iov[iovnum].iov_len  = len;
			txsize += len;
			iovnum++;
		}

		sent = usbip_net_sendvec(sdev->ud.sock_fd, iov, iovnum);
		if (sent != txsize) {
			dev_err(sdev->dev,
				"sendmsg failed!, retval %zd for %zd",
				sent, txsize);
			free(iso_buffer);
			return -1;
		}

		if (iso_buffer)
			free(iso_buffer);

		total_size += txsize;
	}

	pthread_mutex_lock(&sdev->priv_lock);
	list_for_each_safe(pos, tmp, &sdev->priv_free) {
		priv = list_entry(pos, struct stub_priv, list);
		stub_free_priv_and_trx(priv);
	}
	pthread_mutex_unlock(&sdev->priv_lock);

	return total_size;
}

static struct stub_unlink *dequeue_from_unlink_tx(struct stub_device *sdev)
{
	struct list_head *pos, *tmp;
	struct stub_unlink *unlink;

	pthread_mutex_lock(&sdev->priv_lock);

	list_for_each_safe(pos, tmp, &sdev->unlink_tx) {
		unlink = list_entry(pos, struct stub_unlink, list);
		list_del(&unlink->list);
		list_add(&unlink->list, sdev->unlink_free.prev);
		pthread_mutex_unlock(&sdev->priv_lock);
		return unlink;
	}

	pthread_mutex_unlock(&sdev->priv_lock);

	return NULL;
}

static int stub_send_ret_unlink(struct stub_device *sdev)
{
	struct list_head *pos, *tmp;
	struct stub_unlink *unlink;
	struct iovec iov[1];
	size_t txsize;
	size_t total_size = 0;

	while ((unlink = dequeue_from_unlink_tx(sdev)) != NULL) {
		size_t sent;
		struct usbip_header pdu_header;

		txsize = 0;
		memset(&pdu_header, 0, sizeof(pdu_header));
		memset(&iov, 0, sizeof(iov));

		usbip_dbg_stub_tx("setup ret unlink %lu", unlink->seqnum);

		/* 1. setup usbip_header */
		setup_ret_unlink_pdu(&pdu_header, unlink);
		usbip_header_correct_endian(&pdu_header, 1);

		iov[0].iov_base = &pdu_header;
		iov[0].iov_len  = sizeof(pdu_header);
		txsize += sizeof(pdu_header);

		sent = usbip_net_sendvec(sdev->ud.sock_fd, iov, 1);
		if (sent != txsize) {
			dev_err(sdev->dev,
				"sendmsg failed!, retval %zd for %zd",
				sent, txsize);
			return -1;
		}

		usbip_dbg_stub_tx("send txdata");
		total_size += txsize;
	}

	pthread_mutex_lock(&sdev->priv_lock);

	list_for_each_safe(pos, tmp, &sdev->unlink_free) {
		unlink = list_entry(pos, struct stub_unlink, list);
		list_del(&unlink->list);
		free(unlink);
	}

	pthread_mutex_unlock(&sdev->priv_lock);

	return total_size;
}

static int poll_events_and_complete(struct stub_device *sdev)
{
	struct timeval tv = {0, 0};
	int ret;

	ret = libusb_handle_events_timeout(stub_libusb_ctx, &tv);
	if (ret != 0 && ret != LIBUSB_ERROR_TIMEOUT) {
	    dev_err(sdev->dev, "libusb handle events: %d", ret);
        return -1;
	}
    return 0;
}

void *stub_tx_loop(void *data)
{
	struct stub_device *sdev = (struct stub_device *)data;
	int ret_submit, ret_unlink;

	while (!stub_should_stop(sdev)) {
		if (poll_events_and_complete(sdev)) { // TODO move to separate thread?
            break;
		}

		/*
		 * send_ret_submit comes earlier than send_ret_unlink.  stub_rx
		 * looks at only priv_init queue. If the completion of a URB is
		 * earlier than the receive of CMD_UNLINK, priv is moved to
		 * priv_tx queue and stub_rx does not find the target priv. In
		 * this case, vhci_rx receives the result of the submit request
		 * and then receives the result of the unlink request. The
		 * result of the submit is given back to the usbcore as the
		 * completion of the unlink request. The request of the
		 * unlink is ignored. This is ok because a driver who calls
		 * usb_unlink_urb() understands the unlink was too late by
		 * getting the status of the given-backed URB which has the
		 * status of usb_submit_urb().
		 */

		ret_submit = stub_send_ret_submit(sdev);
		if (ret_submit < 0)
			break;

		ret_unlink = stub_send_ret_unlink(sdev);
		if (ret_unlink < 0)
			break;
	}

	sdev->should_stop = 1;

	usbip_dbg_stub_tx("end of stub_tx_loop");

	return NULL;
}
