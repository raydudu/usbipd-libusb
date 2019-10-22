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

#include "stub.h"
#include <usbip_debug.h>
//TODO split the file up and remove

static struct stub_priv *stub_priv_pop_from_listhead(struct list_head *listhead) {
	struct list_head *pos, *tmp;

	list_for_each_safe(pos, tmp, listhead) {
        struct stub_priv *priv = list_entry(pos, struct stub_priv, list);
		warn("Found pending trx %p.", priv->trx);
	}

	return NULL;
}

static struct stub_priv *stub_priv_pop(struct stub_device *sdev) {
	struct stub_priv *priv;

	pthread_mutex_lock(&sdev->priv_lock);

	priv = stub_priv_pop_from_listhead(&sdev->priv_init);
	if (priv)
		goto done;

	priv = stub_priv_pop_from_listhead(&sdev->priv_tx);
	if (priv)
		goto done;

	priv = stub_priv_pop_from_listhead(&sdev->priv_free);

done:
	pthread_mutex_unlock(&sdev->priv_lock);
	return priv;
}

void stub_device_cleanup_transfers(struct stub_device *sdev)
{
	struct stub_priv *priv;
	struct libusb_transfer *trx;

	dev_dbg(sdev->dev, "free sdev %p", sdev);

	while (1) {
		priv = stub_priv_pop(sdev);
		if (!priv)
			break;

		trx = priv->trx;
		libusb_cancel_transfer(trx);

		dev_dbg(sdev->dev, "free trx %p", trx);
		free(priv);
		free(trx->buffer);
		libusb_free_transfer(trx);
	}
}

void stub_device_cleanup_unlinks(struct stub_device *sdev)
{
	/* derived from stub_shutdown_connection */
	struct list_head *pos, *tmp;
	struct stub_unlink *unlink;

	pthread_mutex_lock(&sdev->priv_lock);
	list_for_each_safe(pos, tmp, &sdev->unlink_tx) {
		unlink = list_entry(pos, struct stub_unlink, list);
		list_del(&unlink->list);
		free(unlink);
	}
	list_for_each_safe(pos, tmp, &sdev->unlink_free) {
		unlink = list_entry(pos, struct stub_unlink, list);
		list_del(&unlink->list);
		free(unlink);
	}
	pthread_mutex_unlock(&sdev->priv_lock);
}
